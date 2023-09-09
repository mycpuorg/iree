// RUN: iree-opt --pass-pipeline="builtin.module(func.func(iree-llvmcpu-tile-and-fuse{tiling-level=0}))" --split-input-file %s | FileCheck %s

func.func @matmul_bias_add(%arg0 : tensor<?x?xf32>, %arg1 : tensor<?x?xf32>, %arg2 : tensor<?xf32>) -> tensor<?x?xf32> {
  %cst = arith.constant 0.0 : f32
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %d0 = tensor.dim %arg0, %c0 : tensor<?x?xf32>
  %d1 = tensor.dim %arg1, %c1 : tensor<?x?xf32>
  %init = tensor.empty(%d0, %d1) : tensor<?x?xf32>
  %0 = linalg.fill ins(%cst : f32) outs(%init : tensor<?x?xf32>) -> tensor<?x?xf32>
  %1 = linalg.matmul {lowering_config = #iree_codegen.lowering_config<tile_sizes = [[10, 20, 30]]>}
      ins(%arg0, %arg1 : tensor<?x?xf32>, tensor<?x?xf32>)
      outs(%0 : tensor<?x?xf32>) -> tensor<?x?xf32>
  %2 = linalg.generic {
    indexing_maps = [affine_map<(d0, d1) -> (d0, d1)>, affine_map<(d0, d1) -> (d1)>, affine_map<(d0, d1)-> (d0, d1)>],
    iterator_types = ["parallel", "parallel"]}
    ins(%1, %arg2 : tensor<?x?xf32>, tensor<?xf32>)
    outs(%init : tensor<?x?xf32>) {
      ^bb0(%arg3: f32, %arg4: f32, %arg5: f32):
        %3 = arith.addf %arg3, %arg4 : f32
        linalg.yield %3 : f32
    } -> tensor<?x?xf32>
  return %2 : tensor<?x?xf32>
}
//      CHECK: func.func @matmul_bias_add(
//      CHECK:   scf.for
// CHECK-SAME:   {
//      CHECK:     scf.for
// CHECK-SAME:     {
//      CHECK:       linalg.fill
//      CHECK:       linalg.matmul
//      CHECK:       linalg.generic
//      CHECK:     }
//      CHECK:   }

// -----

func.func @all_zeros(%arg0 : tensor<?x?xf32>, %arg1 : tensor<?x?xf32>, %arg2 : tensor<?xf32>) -> tensor<?x?xf32> {
  %cst = arith.constant 0.0 : f32
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %d0 = tensor.dim %arg0, %c0 : tensor<?x?xf32>
  %d1 = tensor.dim %arg1, %c1 : tensor<?x?xf32>
  %init = tensor.empty(%d0, %d1) : tensor<?x?xf32>
  %0 = linalg.fill ins(%cst : f32) outs(%init : tensor<?x?xf32>) -> tensor<?x?xf32>
  %1 = linalg.matmul {lowering_config = #iree_codegen.lowering_config<tile_sizes = [[0, 0, 0]]>}
      ins(%arg0, %arg1 : tensor<?x?xf32>, tensor<?x?xf32>)
      outs(%0 : tensor<?x?xf32>) -> tensor<?x?xf32>
  %2 = linalg.generic {
    indexing_maps = [affine_map<(d0, d1) -> (d0, d1)>, affine_map<(d0, d1) -> (d1)>, affine_map<(d0, d1)-> (d0, d1)>],
    iterator_types = ["parallel", "parallel"]}
    ins(%1, %arg2 : tensor<?x?xf32>, tensor<?xf32>)
    outs(%init : tensor<?x?xf32>) {
      ^bb0(%arg3: f32, %arg4: f32, %arg5: f32):
        %3 = arith.addf %arg3, %arg4 : f32
        linalg.yield %3 : f32
    } -> tensor<?x?xf32>
  return %2 : tensor<?x?xf32>
}
//      CHECK: func.func @all_zeros(
//  CHECK-NOT:   scf.for
//      CHECK:   linalg.fill
//      CHECK:   linalg.matmul
//      CHECK:   linalg.generic

// -----

func.func @multi_config(%arg0 : tensor<?x?xf32>, %arg1 : tensor<?x?xf32>, %arg2 : tensor<?xf32>) -> tensor<?x?xf32> {
  %cst = arith.constant 0.0 : f32
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %d0 = tensor.dim %arg0, %c0 : tensor<?x?xf32>
  %d1 = tensor.dim %arg1, %c1 : tensor<?x?xf32>
  %init = tensor.empty(%d0, %d1) : tensor<?x?xf32>
  %0 = linalg.fill ins(%cst : f32) outs(%init : tensor<?x?xf32>) -> tensor<?x?xf32>
  %1 = linalg.matmul {lowering_config = #iree_codegen.lowering_config<tile_sizes = [[0, 0]]>}
      ins(%arg0, %arg1 : tensor<?x?xf32>, tensor<?x?xf32>)
      outs(%0 : tensor<?x?xf32>) -> tensor<?x?xf32>
  %2 = linalg.generic {
    indexing_maps = [affine_map<(d0, d1) -> (d0, d1)>, affine_map<(d0, d1) -> (d1)>, affine_map<(d0, d1)-> (d0, d1)>],
    iterator_types = ["parallel", "parallel"]}
    ins(%1, %arg2 : tensor<?x?xf32>, tensor<?xf32>)
    outs(%init : tensor<?x?xf32>) attrs = {lowering_config = #iree_codegen.lowering_config<tile_sizes = [[10, 20]]>} {
      ^bb0(%arg3: f32, %arg4: f32, %arg5: f32):
        %3 = arith.addf %arg3, %arg4 : f32
        linalg.yield %3 : f32
    } -> tensor<?x?xf32>
  return %2 : tensor<?x?xf32>
}
// Both linalg.matmul op and linalg.generic op have lowering_config. Test that
// the lowering_config of linalg.generic op is picked in the pass. In this case,
// scf.for ops are created. If the lowering_config of linalg.matmul op is
// picked, there are no scf.for ops. Because the tiling sizes are zeros.
//      CHECK: func.func @multi_config(
//      CHECK:   scf.for
//      CHECK:     scf.for
//  CHECK-NOT:       scf.for
//      CHECK:       linalg.fill
//      CHECK:       linalg.matmul
//      CHECK:       linalg.generic

// -----

func.func @shared_out_operand() {
  %cst = arith.constant 0.000000e+00 : f32
  %cst_0 = arith.constant 6.000000e+00 : f32
  %c600576 = arith.constant 600576 : index
  %c0 = arith.constant 0 : index
  %0 = hal.interface.constant.load[0] : i32
  %1 = hal.interface.constant.load[1] : i32
  %2 = arith.index_castui %0 {stream.alignment = 1024 : index, stream.values = [205824 : index, 795648 : index, 1385472 : index, 1975296 : index, 2565120 : index, 3154944 : index, 3744768 : index]} : i32 to index
  %3 = arith.index_castui %1 {stream.alignment = 1024 : index, stream.values = [0 : index, 3072 : index, 6144 : index, 9216 : index, 12288 : index, 15360 : index, 18432 : index]} : i32 to index
  %4 = hal.interface.binding.subspan set(0) binding(0) type(storage_buffer) alignment(64) offset(%c0) flags(ReadOnly) : !flow.dispatch.tensor<readonly:tensor<391x384xf32>>
  %5 = hal.interface.binding.subspan set(0) binding(1) type(storage_buffer) alignment(64) offset(%2) flags(ReadOnly) : !flow.dispatch.tensor<readonly:tensor<384x384xf32>>
  %6 = hal.interface.binding.subspan set(0) binding(1) type(storage_buffer) alignment(64) offset(%3) flags(ReadOnly) : !flow.dispatch.tensor<readonly:tensor<384xf32>>
  %7 = hal.interface.binding.subspan set(0) binding(2) type(storage_buffer) alignment(64) offset(%c600576) : !flow.dispatch.tensor<writeonly:tensor<391x384xf32>>
  %8 = flow.dispatch.tensor.load %4, offsets = [0, 0], sizes = [391, 384], strides = [1, 1] : !flow.dispatch.tensor<readonly:tensor<391x384xf32>> -> tensor<391x384xf32>
  %9 = flow.dispatch.tensor.load %5, offsets = [0, 0], sizes = [384, 384], strides = [1, 1] : !flow.dispatch.tensor<readonly:tensor<384x384xf32>> -> tensor<384x384xf32>
  %10 = flow.dispatch.tensor.load %6, offsets = [0], sizes = [384], strides = [1] : !flow.dispatch.tensor<readonly:tensor<384xf32>> -> tensor<384xf32>
  %11 = flow.dispatch.tensor.load %7, offsets = [0, 0], sizes = [391, 384], strides = [1, 1] : !flow.dispatch.tensor<writeonly:tensor<391x384xf32>> -> tensor<391x384xf32>
  %12 = linalg.fill ins(%cst : f32) outs(%11 : tensor<391x384xf32>) -> tensor<391x384xf32>
  %13 = linalg.matmul {lowering_config = #iree_codegen.lowering_config<tile_sizes = [[8, 128, 0]]>}
    ins(%8, %9 : tensor<391x384xf32>, tensor<384x384xf32>)
    outs(%12 : tensor<391x384xf32>) -> tensor<391x384xf32>
  %14 = linalg.generic {
      indexing_maps = [affine_map<(d0, d1) -> (d1)>,
                     affine_map<(d0, d1) -> (d0, d1)>,
                     affine_map<(d0, d1) -> (d0, d1)>],
      iterator_types = ["parallel", "parallel"]
    }
    ins(%10, %13 : tensor<384xf32>, tensor<391x384xf32>)
    outs(%11 : tensor<391x384xf32>) {
  ^bb0(%in: f32, %in_1: f32, %out: f32):
    %15 = arith.addf %in, %in_1 : f32
    %16 = arith.minf %15, %cst_0 : f32
    %17 = arith.maxf %16, %cst : f32
    linalg.yield %17 : f32
  } -> tensor<391x384xf32>
  flow.dispatch.tensor.store %14, %7, offsets = [0, 0], sizes = [391, 384], strides = [1, 1] : tensor<391x384xf32> -> !flow.dispatch.tensor<writeonly:tensor<391x384xf32>>
  return
}
//      CHECK: func.func @shared_out_operand(
//  CHECK-DAG:   %[[CST0:.+]] = arith.constant 0.000000e+00 : f32
//  CHECK-DAG:   %[[DST_BINDING:.+]] = hal.interface.binding.subspan {{.+}} : !flow.dispatch.tensor<writeonly:tensor<391x384xf32>>
//      CHECK:   %[[DST:.+]] = flow.dispatch.tensor.load %[[DST_BINDING]]
//      CHECK:   scf.for
// CHECK-SAME:       iter_args(%[[ITER0:.+]] = %[[DST]])
//      CHECK:     scf.for
// CHECK-SAME:       iter_args(%[[ITER1:.+]] = %[[ITER0]])
//      CHECK:       %[[OUT_SLICE:.+]] = tensor.extract_slice %[[ITER1]]
//      CHECK:       %{{.+}} = linalg.fill ins(%[[CST0]] : f32) outs(%[[OUT_SLICE]]
//      CHECK:       %{{.+}} = linalg.matmul
//      CHECK:       %[[OUT_SLICE2:.+]] = tensor.extract_slice %[[ITER1]]
//      CHECK:       %{{.+}} = linalg.generic
// CHECK-SAME:         outs(%[[OUT_SLICE2]]
