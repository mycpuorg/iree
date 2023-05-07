import enum
import os.path
import shutil
import functools
import operator
import collections
import subprocess
from library import *
from dispatch import *


################################################################################
class MatmulOperation:
  """Data structure to describe a matrix multiplication operation. 
     This includes the shape, datatype, and layout of the operands. This data 
     structure is *independent* of the compilation passes and tiling configuration. 
     It strictly contains the parameter that changes the functionality of matmul 
     operation.
  """

  def __init__(self, problem_shape, lhs, rhs, result):
    """Initializes a matrix multiplication operation.
    Matrix-multiple operation: `result[M, N] = lhs[M, K] * rhs[K, N]`
    problem_shape: A tuple representing the matrix multiplication problem shape
      in the format (M, N, K), where M is the number of rows in the lhs matrix, 
      N is the number of columns in the rhs matrix, and K is the number of columns 
      in the lhs matrix and rows in the rhs matrix.
    lhs: A TensorDescription object representing the left-hand-side matrix operand.
    rhs: A TensorDescription object representing the right-hand-side matrix operand.
    result: A TensorDescription object representing the result matrix operand.
    """
    self.problem_shape = problem_shape  # Matmul problem shape [M, N, K]
    self.M = problem_shape[0]
    self.N = problem_shape[1]
    self.K = problem_shape[2]

    self.lhs = lhs  # TensorDescription
    self.rhs = rhs  # TensorDescription
    self.result = result  # TensorDescription
    self.operation_kind = OperationKind.Matmul

  def __eq__(self, other):
    return self.problem_shape == other.problem_shape and \
           self.lhs == other.lhs and \
           self.rhs == other.rhs and \
           self.result == other.result

  def name(self):
    """Procedurally generated name for the matmul operation.
    The name uniquely identifies a matmul operation with matmul shape, 
    lhs dataype and layout, rhs datatype and layout, and result
    datatype and layout.
    """
    return "matmul_{m}x{n}x{k}_"\
      "{typeLhs}{layoutLhs}_"\
      "{typeRhs}{layoutRhs}_"\
      "{typeResult}{layoutResult}".format(
      m=self.problem_shape[0],
      n=self.problem_shape[1],
      k=self.problem_shape[2],
      typeLhs=DataTypeName[self.lhs.datatype],
      layoutLhs=ShortLayoutTypeName[self.lhs.layout],
      typeRhs=DataTypeName[self.rhs.datatype],
      layoutRhs=ShortLayoutTypeName[self.rhs.layout],
      typeResult=DataTypeName[self.result.datatype],
      layoutResult=ShortLayoutTypeName[self.result.layout]
    )

  def lhs_npy_shape(self):
    """Returns the shape of the lhs numpy array as a string in the format "MxKxDataType"."""
    return f"{self.M}x{self.K}x{DataTypeName[self.lhs.datatype]}"

  def rhs_npy_shape(self):
    """Returns the shape of the rhs numpy array as a string in the format "KxNxDataType"."""
    return f"{self.K}x{self.N}x{DataTypeName[self.rhs.datatype]}"

  def result_npy_shape(self):
    """Returns the shape of the result numpy array as a string in the format "MxNxDataType"."""
    return f"{self.M}x{self.N}x{DataTypeName[self.result.datatype]}"

  def bytes(self):
    """Returns the number of bytes read/written by the matmul operation."""
    bytes = (DataTypeSizeInBits[self.lhs.datatype] * self.M // 8) * self.K + \
            (DataTypeSizeInBits[self.rhs.datatype] * self.K // 8) * self.N + \
            (DataTypeSizeInBits[self.result.datatype] * self.M // 8) * self.N
    return bytes

  def flops(self):
    """Returns the number of floating point operations performed by the matmul operation."""
    return 2 * self.M * self.N * self.K


##############################################################################
class MatmulCompilationInfo:
  """Data structure strictly describes the compilation passes and the tiling configurations. 
  For a matrix multiplication operation, compilation passes and tiling configuration 
  influences the performance of the compiled matmul operation, but the functionality. 
  This data structure should be independent of the matmul operation functionality. 
  
  Any change in this data structure should not affect the functionality of the matmul operation, i.e., 
  we should be able to use the same reference results for a matrix operation compiled with different 
  compilation info.
  """

  def __init__(self,
               tile_description,
               translation_info,
               config_type=CompilationConfigType.Custom):
    self.tile_description = tile_description  # TileDescription
    self.translation_info = translation_info  # TranslationInfo
    self.config_type = config_type  # CompilationConfigType

  def name(self):
    """Procedurally generated name for the matmul compilation info."""
    if self.config_type == CompilationConfigType.Default:
      return "tile_config_default"

    return "tile_config_{tbm}x{tbn}_{tbk}x{stages}_{translation_info}".format(
        tbm=self.tile_description.threadblock_shape[0],
        tbn=self.tile_description.threadblock_shape[1],
        tbk=self.tile_description.threadblock_shape[2],
        stages=self.tile_description.stages,
        translation_info=TranslationInfoName[self.translation_info])


################################################################################
class EmitMatmulCompilationInfo:
  """Emitters for the matmul compilation info."""

  def __init__(self):
    self.compilation_info_template = """
// matmul compilation info (tile configuration, translation info, workgroup size)
#${compilation_info_name} = #iree_codegen.compilation_info<
  lowering_config = <tile_sizes = [[${threadblock_shape_m}, ${threadblock_shape_n}, ${threadblock_shape_k}]]>,
  translation_info = <${translation_info} pipeline_depth = ${stages}>,
  workgroup_size = [${block_dim_x} : index, ${block_dim_y} : index, ${block_dim_z} : index]
>
"""

  def emit(self, compilation_info):
    """Emits the matmul compilation info as a string."""
    if compilation_info.config_type == CompilationConfigType.Default:
      return ""

    values = {
        'compilation_info_name':
            compilation_info.name(),
        'translation_info':
            TranslationInfoTag[compilation_info.translation_info],
        'threadblock_shape_m':
            str(compilation_info.tile_description.threadblock_shape[0]),
        'threadblock_shape_n':
            str(compilation_info.tile_description.threadblock_shape[1]),
        'threadblock_shape_k':
            str(compilation_info.tile_description.threadblock_shape[2]),
        'stages':
            str(compilation_info.tile_description.stages),
        'block_dim_x':
            str(compilation_info.tile_description.block_dim[0]),
        'block_dim_y':
            str(compilation_info.tile_description.block_dim[1]),
        'block_dim_z':
            str(compilation_info.tile_description.block_dim[2]),
    }

    return SubstituteTemplate(self.compilation_info_template, values)


###############################################################################
class EmitLinalgMatmulDispatch:
  """Emitters for the `linalg.matmul` dispatch."""

  def __init__(self):
    self.mlir_dialect = MlirDialect.Linalg

    self.linalg_row_row_matmul_template = """
// Dispatch linalg.matmul row-row layout 
func.func @${operation_name}_${compilation_info_name}(
  %lhs: tensor<${problem_m}x${problem_k}x${datatype_lhs}>,
  %rhs: tensor<${problem_k}x${problem_n}x${datatype_rhs}>) -> tensor<${problem_m}x${problem_n}x${datatype_result}>
{
  %c0 = arith.constant 0.0 : ${datatype_result}
  %init = tensor.empty() : tensor<${problem_m}x${problem_n}x${datatype_result}>
  %inital_result = linalg.fill ins(%c0 : ${datatype_result}) outs(%init : tensor<${problem_m}x${problem_n}x${datatype_result}>) -> tensor<${problem_m}x${problem_n}x${datatype_result}>
  %result = linalg.matmul ${compilation_info_attribute} 
                     ins(%lhs, %rhs: tensor<${problem_m}x${problem_k}x${datatype_lhs}>, tensor<${problem_k}x${problem_n}x${datatype_rhs}>)
                     outs(%inital_result: tensor<${problem_m}x${problem_n}x${datatype_result}>) -> tensor<${problem_m}x${problem_n}x${datatype_result}>
  return %result : tensor<${problem_m}x${problem_n}x${datatype_result}>
}
"""

  def emit(self, matmul_dispatch):
    """Emit the matmul operation in the MLIR dialect for a single compilation info"""
    compilation_info_attribute_template = """{compilation_info = #${compilation_info_name}}"""
    compilation_info_attribute_str = SubstituteTemplate(
        compilation_info_attribute_template,
        {'compilation_info_name': matmul_dispatch.configuration.name()})
    compilation_info_attribute = compilation_info_attribute_str \
      if matmul_dispatch.configuration.config_type != CompilationConfigType.Default else ""

    values = {
        'operation_name':
            matmul_dispatch.operation.name(),
        'compilation_info_attribute':
            compilation_info_attribute,
        'problem_m':
            str(matmul_dispatch.operation.problem_shape[0]),
        'problem_n':
            str(matmul_dispatch.operation.problem_shape[1]),
        'problem_k':
            str(matmul_dispatch.operation.problem_shape[2]),
        'datatype_lhs':
            DataTypeName[matmul_dispatch.operation.lhs.datatype],
        'datatype_rhs':
            DataTypeName[matmul_dispatch.operation.rhs.datatype],
        'datatype_result':
            DataTypeName[matmul_dispatch.operation.result.datatype],
        'compilation_info_name':
            matmul_dispatch.configuration.name()
    }

    return SubstituteTemplate(self.linalg_row_row_matmul_template, values)


# Emit `mhlo.matmul` operation.
# TODO: Add support for testing lowering matmul op from other dialect.
class EmitMhloMatmulOperation:

  def __init__(self):
    self.mlir_dialect = MlirDialect.Mhlo

    self.linalg_row_row_matmul_template = """
// mhlo.matmul operation row-row layout
"""


###############################################################################
class ReferenceMatmulOperation:
  """Reference implementation for the matmul operation in numpy.
      ReferenceMatmulOperation class has the following responsibilities:
       1) Generates matmul operation inputs as np.array for a desired 
          distribution.
       2) Runs the matmul reference operation in np.matmul.
       3) Generates the matmul operation expected output as np.array.
       4) Additional, generate input and output filename strings.
  """

  def __init__(self, matmul_operation, dist_lhs, dist_rhs):
    self.matmul_operation = matmul_operation
    # Problem shape.
    self.M = matmul_operation.problem_shape[0]
    self.N = matmul_operation.problem_shape[1]
    self.K = matmul_operation.problem_shape[2]

    # Data type for the input and result matrices.
    self.dtype_lhs = DataTypeNumPyTag[matmul_operation.lhs.datatype]
    self.dtype_rhs = DataTypeNumPyTag[matmul_operation.rhs.datatype]
    self.dtype_result = DataTypeNumPyTag[matmul_operation.result.datatype]

    # Distribution of the input tensors.
    self.dist_lhs = dist_lhs
    self.dist_rhs = dist_rhs

    # Filename for the left hand side input tensor.
    self.filename_lhs = "m{problem_m}xk{problem_k}_"\
      "{tensor_description}_{dist}_lhs.npy".format(
      problem_m=self.M,
      problem_k=self.K,
      tensor_description=self.matmul_operation.lhs.name(),
      dist=DistributionName[self.dist_lhs])

    # Filename for the right hand side input tensor.
    self.filename_rhs = "k{problem_k}xn{problem_n}_"\
      "{tensor_description}_{dist}_rhs.npy".format(
      problem_k=self.K,
      problem_n=self.N,
      tensor_description=self.matmul_operation.rhs.name(),
      dist=DistributionName[self.dist_rhs])

    # Filename for the reference result tensor.
    self.reference_filename_result = "m{problem_m}xn{problem_n}_"\
      "{tensor_description}_reference_result.npy".format(
      problem_m=self.M,
      problem_n=self.N,
      tensor_description=self.matmul_operation.result.name())

  # Generates input data, runs reference numpy.matmul, and save npy files to the output directory.
  def run_and_save(self, output_dir="."):

    # Generate the input data as np.array for the matmul operation.
    lhs_np_array = get_np_array(self.matmul_operation.lhs, (self.M, self.K),
                                self.dist_lhs)
    rhs_np_array = get_np_array(self.matmul_operation.rhs, (self.K, self.N),
                                self.dist_rhs)

    # Run the reference np.matmul and generate result np.array.
    result = np.matmul(lhs_np_array, rhs_np_array)

    # Save the input data as np.array for the matmul operation.
    np.save(os.path.join(output_dir, self.filename_lhs),\
             np.array(lhs_np_array, dtype = self.dtype_lhs))
    np.save(os.path.join(output_dir, self.filename_rhs),\
             np.array(rhs_np_array, dtype = self.dtype_rhs))

    # Save the expected result as an np.array.
    np.save(os.path.join(output_dir, self.reference_filename_result),\
             np.array(result, dtype = self.dtype_result))


###############################################################################
class MatmulOperationLauncher:
  """Launches the compilation and execution of the matmul operation.
  MatmulOperationLauncher class has the following responsibilities:
    1) Launches compilation of the matmul for a verification or profiling runs.
    2) Launches the verification by running the IREE compiled matmul operation
       and the python nmpy.matmul.
    3) Launches the profiling by running the IREE compiled matmul operation.
  """

  def __init__(self, args, operation):
    self.operation = operation

    # Variables from top-level argparse.
    self.generated_path = os.path.join(args.build_dir, 'generated',
                                       args.mlir_dialect)
    self.args = args
    self.benchmark_dispatch_repeat_count = args.batch_size
    self.batch_size = args.batch_size

    # Additional paths.
    self.matmul_path = os.path.join(self.generated_path, 'matmul')
    self.operation_path = os.path.join(self.matmul_path, operation.name())
    self.source_mlir_file = os.path.join(self.operation_path,
                                         operation.name() + '.mlir')

    # path to iree-compile tool. (for compiling the input mlir file to vmfb)
    self.iree_compile_path = os.path.join(args.build_dir, 'tools',
                                          'iree-compile')

    # path to iree-benchmark-module tool. (for performance benchmarking and profiling)
    self.iree_benchmark_module_path = os.path.join(args.build_dir, 'tools',
                                                   'iree-benchmark-module')

    # path to iree-run-module tool. (for verification)
    self.iree_run_module_path = os.path.join(args.build_dir, 'tools',
                                             'iree-run-module')

    # output vmfb files for the operation.
    self.vmfb_verify_file = os.path.join(self.operation_path,
                                         self.operation.name() + '_verify.vmfb')
    self.vmfb_benchmark_file = os.path.join(
        self.operation_path,
        self.operation.name() + '_benchmark.vmfb')

  def compile(self, compilation_mode):
    """Compiles the matmul operation to a vmfb file for profiling."""

    benchmark_dispatch_repeat_count = self.benchmark_dispatch_repeat_count if compilation_mode == CompilationMode.Profile else 1
    vmfb_file = self.vmfb_benchmark_file if compilation_mode == CompilationMode.Profile else self.vmfb_verify_file

    # Base iree-compile commandline
    cmd = [self.iree_compile_path, self.source_mlir_file, "-o", f"{vmfb_file}"]

    # General compilation options
    cmd += [f"--iree-hal-target-backends={self.args.device}"]
    cmd += [f"--iree-hal-cuda-llvm-target-arch={self.args.cuda_arch}"]
    if self.args.split_k_slices != "":
      cmd += [f"--iree-flow-split-matmul-reduction={self.args.split_k_slices}"]
    if self.args.use_mma_sync:
      cmd += [f"--iree-codegen-llvmgpu-use-mma-sync"]
    if self.args.use_wmma:
      cmd += [f"--iree-codegen-llvmgpu-use-wmma"]

    # Compilation options for profiling
    cmd += [
        f"--iree-hal-benchmark-dispatch-repeat-count={benchmark_dispatch_repeat_count}"
    ]

    if not os.path.exists(vmfb_file) or self.args.force_compile:
      print(
          f">> Compilation command for {CompilationModeNames[compilation_mode]} : {' '.join(cmd)}"
      )
      subprocess.check_output(cmd)

    elif self.args.verbose:
      print("Skipping compilation of matmul operation: " + vmfb_file +
            " since it already exists.")

  def verify(self, configuration):
    """Verifies the matmul operation with a given configuration."""
    # First compile the operation to a vmfb file.
    self.compile(CompilationMode.Verify)

    # Verify using random data distribution.
    # TODO 1) make input distribution configurable through command line.
    # TODO 2) make the reference run to check if reference npy files are present,
    #         then do not re-run the reference.
    reference_op = ReferenceMatmulOperation(self.operation,\
                                         Distribution.Random,\
                                         Distribution.Random)

    lhs_npy_file = os.path.join(self.operation_path, reference_op.filename_lhs)
    rhs_npy_file = os.path.join(self.operation_path, reference_op.filename_rhs)
    expected_result_npy_file = os.path.join(
        self.operation_path, reference_op.reference_filename_result)

    # If the reference numpy do not exists, run the reference implementation
    # and generate npy files.
    if not os.path.exists(lhs_npy_file) or \
       not os.path.exists(rhs_npy_file) or \
       not os.path.exists(expected_result_npy_file):
      reference_op.run_and_save(self.operation_path)

    # Commandline `iree-run-module` for verification.
    cmd = [
        self.iree_run_module_path, f'--module={self.vmfb_verify_file}',
        f'--device={self.args.device}'
    ]

    # Operation-specific verification command-line.
    # TODO: abstract the operation-specific verification command-line out of verification.
    cmd.append(f'--function={self.operation.name()}_{configuration.name()}')
    cmd.append(f'--input=@{lhs_npy_file}')
    cmd.append(f'--input=@{rhs_npy_file}')
    cmd.append(f'--expected_output=@{expected_result_npy_file}')

    # Print the command if verbose.
    if self.args.verbose:
      print(">> Verification command: " + ' '.join(cmd))

    # Launch verification.
    cmd_output = subprocess.check_output(cmd, text=True)

    # Parse the verification output.
    m = re.search(r"\[(?P<verification_result>[a-zA-Z]+)\]", cmd_output)
    if m is None:
      raise Exception(
          "Failed to parse verification output by iree-run-module: " +
          cmd_output)
    verification_result = m.group('verification_result')

    if self.args.verbose or verification_result != "SUCCESS":
      print(cmd_output)

    return verification_result

  def profile(self, configuration):
    """Profiles the matmul operation with a given configuration."""
    # First compile the operation to a vmfb file.
    self.compile(CompilationMode.Profile)

    # Commandline `iree-benchmark-module` for profiling.
    cmd = [
        self.iree_benchmark_module_path, f'--module={self.vmfb_benchmark_file}',
        f'--device={self.args.device}'
    ]

    # Profiling specific flags.
    cmd += [f'--benchmark_repetitions={self.args.benchmark_repetitions}']
    cmd += [f'--batch_size={self.batch_size}']

    # Operation-specific profiling command-line.
    cmd += [f'--function={self.operation.name()}_{configuration.name()}']
    cmd += [f'--input={self.operation.lhs_npy_shape()}']
    cmd += [f'--input={self.operation.rhs_npy_shape()}']

    # Print the command if verbose.
    if self.args.verbose:
      print(">> Profiling command: " + ' '.join(cmd))

    # Launch profiling.
    cmd_output = subprocess.check_output(cmd,
                                         text=True,
                                         stderr=subprocess.STDOUT)

    # Parse the profiling output.
    m = re.search(r"real_time_median\s+(?P<runtime>\d+.\d+)\s+ms", cmd_output)
    if m is None:
      raise Exception("Failed to parse runtime from benchmark result: " +
                      cmd_output)
    runtime_in_ms = float(m.group('runtime'))
    return runtime_in_ms


##############################################################################
class MatmulGenerator:
  """Matmul dispatch generator class.
  Generates a list of pre-definied matmul operations with resonable tuning cofigurations. 
  The generator function are seperated based on the target backend and the data type.
  Please see example `MatmulGenerator._cuda_matmul_tensor_cores_f16` for cuda target 
  backend and f16 data type."""

  def __init__(self, args):
    self.args = args

    self.default_config = False if args.default_config in [
        'False', 'false', '0'
    ] else True

    self.translation_infos = [
        #TranslationInfo.LLVMGPUMatmulSimt,  # CUDA Core (SMIT)
        #TranslationInfo.LLVMGPUMatmulTensorCore, # Tensor Core (WMMA)
        TranslationInfo.
        LLVMGPUMatmulTensorCoreMmaSync,  # Tensor Core (MMA.SYNC)
    ]

    self.problem_shapes = [[128, 256, 8192]]
    """
    self.problem_shapes = [[128, 128, 256], [256, 512, 128], [1024, 512, 2048],
                           [2560, 2560, 2560], [3456, 1024, 2048]]
    """

    # List of pre-definied matmul dispatch collections.
    self.dispatches_collection_list = []

    # CUDA specific constants.
    self.cuda_warp_size = 32
    self.cuda_smem_capacity_in_bytes_sm80 = 192 << 10

  def _is_tile_aligned_shape(self, dispatch):
    """Checks if the given dispatch is valid for CUDA."""
    problem_shape = dispatch.operation.problem_shape
    threadblock_shape = dispatch.configuration.tile_description.threadblock_shape
    if len(problem_shape) != len(threadblock_shape):
      raise ValueError("Problem shape and threadblock shape must have the "\
                       "same rank.")
    is_aligned = all(
        a % b == 0 for a, b in zip(problem_shape, threadblock_shape))
    return is_aligned

  def _cuda_smem_required_in_bytes(self, dispatch):
    """Returns size bytes of shared memory required for a given cuda dispatch."""
    threadblock_shape = dispatch.configuration.tile_description.threadblock_shape
    num_stages = dispatch.configuration.tile_description.stages
    tile_shape_lhs = threadblock_shape[0] * threadblock_shape[2]
    tile_shape_rhs = threadblock_shape[2] * threadblock_shape[1]
    return ((tile_shape_lhs * DataTypeSizeInBits[dispatch.operation.lhs.datatype] + \
             tile_shape_rhs * DataTypeSizeInBits[dispatch.operation.rhs.datatype]) * num_stages) // 8

  def _is_cuda_smem_avialable(self, dispatch):
    """Checks if the given dispatch is valid for CUDA."""
    return self._cuda_smem_required_in_bytes(
        dispatch) <= self.cuda_smem_capacity_in_bytes_sm80

  def _cuda_supported_configuration_list(self, operation, configuration_list):
    """Returns a list of supported configurations for CUDA."""
    supported_configuration_list = []
    for configuration in configuration_list:
      dispatch = Dispatch(operation, configuration)
      if not self._is_tile_aligned_shape(dispatch):
        print(f"Warning: {dispatch.name()} is not aligned is being skipped.")
        continue
      if not self._is_cuda_smem_avialable(dispatch):
        print(f"Warning: {dispatch.name()} requires {self._cuda_smem_required_in_bytes(dispatch)} "\
              f"bytes of shared memory, which is larger than the SM80 capacity "\
              f"{self.cuda_smem_capacity_in_bytes_sm80} bytes.")
        continue

      # If all checks pass, add the configuration to the supported list.
      supported_configuration_list.append(configuration)

    return supported_configuration_list

  def _cuda_matmul_tensor_cores_f16(self):
    """Appends a list of matmul dispatches for GPU TensorCore F16 data type."""

    tile_descriptions = [
        TileDescription([256, 128, 32], 3, [64, 4, 1]),
        TileDescription([128, 256, 32], 3, [128, 2, 1]),
        TileDescription([128, 128, 64], 4, [64, 2, 1]),
        TileDescription([128, 128, 32], 5, [64, 2, 1]),
        TileDescription([128, 64, 32], 5, [64, 2, 1]),
        TileDescription([64, 64, 64], 5, [64, 2, 1]),
        TileDescription([64, 64, 32], 10, [64, 2, 1]),
    ]

    # Create configuration list from the tile descriptions and translation infos.
    configuration_list = []

    for tile_description in tile_descriptions:
      for translation_info in self.translation_infos:
        configuration_list.append(
            MatmulCompilationInfo(tile_description, translation_info))

    # Create dispatches collection for each problem shape with the configuration list.
    for problem_shape in self.problem_shapes:
      operation = MatmulOperation(
        problem_shape,\
        TensorDescription(DataType.f16, LayoutType.RowMajor), \
        TensorDescription(DataType.f16, LayoutType.RowMajor), \
        TensorDescription(DataType.f16, LayoutType.RowMajor))

      # Filter out configurations that are not supported by LLVM GPU CUDA backend.
      supported_configuration_list = self._cuda_supported_configuration_list(
          operation, configuration_list)

      # Add default configuration if requested.
      if self.default_config:
        supported_configuration_list.append(
            MatmulCompilationInfo([], [], CompilationConfigType.Default))

      self.dispatches_collection_list.append(DispatchCollection(\
        operation, supported_configuration_list))

  ################################################################################
  def _cuda_matmul_tensor_cores_f32(self):
    """Appends a list of matmul dispatches for GPU TensorCore F32 data type."""

    tile_descriptions = [
        TileDescription([128, 256, 16], 3, [128, 2, 1]),
        TileDescription([256, 128, 16], 3, [64, 4, 1]),
        TileDescription([128, 128, 16], 5, [64, 2, 1]),
        TileDescription([128, 128, 32], 3, [64, 2, 1]),
        TileDescription([128, 128, 32], 4, [64, 2, 1]),
        TileDescription([64, 64, 64], 3, [64, 2, 1]),
    ]

    # Create dispatches collection for each problem shape with the configuration list.
    configuration_list = []

    for tile_description in tile_descriptions:
      for translation_info in self.translation_infos:
        configuration_list.append(
            MatmulCompilationInfo(tile_description, translation_info))

    for problem_shape in self.problem_shapes:
      operation = MatmulOperation(
        problem_shape,\
        TensorDescription(DataType.f32, LayoutType.RowMajor), \
        TensorDescription(DataType.f32, LayoutType.RowMajor), \
        TensorDescription(DataType.f32, LayoutType.RowMajor))

      # Filter out configurations that are not supported by LLVM GPU CUDA backend.
      supported_configuration_list = self._cuda_supported_configuration_list(
          operation, configuration_list)

      # Add default configuration if requested.
      if self.default_config:
        supported_configuration_list.append(
            MatmulCompilationInfo([], [], CompilationConfigType.Default))

      self.dispatches_collection_list.append(DispatchCollection(\
        operation, supported_configuration_list))

  def generate(self):
    """Generates a list of matmul operations."""
    if self.args.device == 'cuda':
      self._cuda_matmul_tensor_cores_f16()
      self._cuda_matmul_tensor_cores_f32()
    return self.dispatches_collection_list


##############################################################################
