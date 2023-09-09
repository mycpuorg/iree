---
hide:
  - tags
tags:
  - GPU
  - CUDA
icon: octicons/server-16
---

# GPU deployment using CUDA and ROCm

IREE can accelerate model execution on Nvidia GPUs using CUDA and on AMD GPUs
using ROCm. Due to the similarity of CUDA and ROCm APIs and infrastructure, the
CUDA and ROCm backends share much of their implementation in IREE:

* The IREE compiler uses a similar GPU code generation pipeline for each, but
  generates PTX for CUDA and hsaco for ROCm
* The IREE runtime HAL driver for ROCm mirrors the one for CUDA, except for
  command buffers implementations - where CUDA has "direct", "stream", and
  "graph" command buffers, and ROCm has only "direct" command buffers

## :octicons-download-16: Prerequisites

In order to use CUDA or ROCm to drive the GPU, you need to have a functional
CUDA or ROCm environment. It can be verified by the following steps:

=== "Nvidia/CUDA"

    Run the following command in a shell:

    ``` shell
    nvidia-smi | grep CUDA
    ```

    If `nvidia-smi` does not exist, you will need to
    [install the latest CUDA Toolkit SDK](https://developer.nvidia.com/cuda-downloads).

=== "AMD/ROCm"

    Run the following command in a shell:

    ``` shell
    rocm-smi | grep rocm
    ```

    If `rocm-smi` does not exist, you will need to
    [install the latest ROCm Toolkit SDK](https://rocmdocs.amd.com/en/latest/Installation_Guide/Installation_new.html)).

### Get the IREE compiler

#### :octicons-package-16: Download the compiler from a release

=== "Nvidia/CUDA"

    Python packages are regularly published to
    [PyPI](https://pypi.org/user/google-iree-pypi-deploy/). See the
    [Python Bindings](../../reference/bindings/python.md) page for more details.
    The core `iree-compiler` package includes the CUDA compiler:

    === "Stable releases"

        Stable release packages are
        [published to PyPI](https://pypi.org/user/google-iree-pypi-deploy/).

        ``` shell
        python -m pip install iree-compiler
        ```

    === ":material-alert: Nightly releases"

        Nightly releases are published on
        [GitHub releases](https://github.com/openxla/iree/releases).

        ``` shell
        python -m pip install \
          --find-links https://openxla.github.io/iree/pip-release-links.html \
          --upgrade iree-compiler
        ```

    !!! tip
        `iree-compile` is installed to your python module installation path. If you
        pip install with the user mode, it is under `${HOME}/.local/bin`, or
        `%APPDATA%Python` on Windows. You may want to include the path in your
        system's `PATH` environment variable:

        ```shell
        export PATH=${HOME}/.local/bin:${PATH}
        ```

=== "AMD/ROCm"

    Currently ROCm is **NOT supported** for the Python interface.

#### :material-hammer-wrench: Build the compiler from source

Please make sure you have followed the
[Getting started](../../building-from-source/getting-started.md) page to build
the IREE compiler, then enable the CUDA compiler target with the
`IREE_TARGET_BACKEND_CUDA` option or the ROCm compiler target with the
`IREE_TARGET_BACKEND_ROCM` option.

!!! tip
    `iree-compile` will be built under the `iree-build/tools/` directory. You
    may want to include this path in your system's `PATH` environment variable.

### Get the IREE runtime

Next you will need to get an IREE runtime that includes the CUDA (for Nvidia
hardware) or ROCm (for AMD hardware) HAL driver.

#### :material-hammer-wrench: Build the runtime from source

Please make sure you have followed the
[Getting started](../../building-from-source/getting-started.md) page to build
IREE from source, then enable the CUDA HAL driver with the
`IREE_HAL_DRIVER_CUDA` option or the experimental ROCm HAL driver with the
`IREE_EXTERNAL_HAL_DRIVERS=rocm` option.

## Compile and run a program model

With the compiler and runtime ready, we can now compile programs and run them
on GPUs.

### :octicons-file-code-16: Compile a program

The IREE compiler transforms a model into its final deployable format in many
sequential steps. A model authored with Python in an ML framework should use the
corresponding framework's import tool to convert into a format (i.e.,
[MLIR](https://mlir.llvm.org/)) expected by the IREE compiler first.

Using MobileNet v2 as an example, you can download the SavedModel with trained
weights from
[TensorFlow Hub](https://tfhub.dev/google/tf2-preview/mobilenet_v2/classification)
and convert it using IREE's
[TensorFlow importer](../ml-frameworks/tensorflow.md). Then run one of the
following commands to compile:

=== "Nvidia/CUDA"

    ```shell hl_lines="2-3"
    iree-compile \
        --iree-hal-target-backends=cuda \
        --iree-hal-cuda-llvm-target-arch=<...> \
        mobilenet_iree_input.mlir -o mobilenet_cuda.vmfb
    ```

    Note that a cuda target architecture(`iree-hal-cuda-llvm-target-arch`) of
    the form `sm_<arch_number>` is needed to compile towards each GPU
    architecture. If no architecture is specified then we will default to
    `sm_35`.

    Here is a table of commonly used architectures:

    | CUDA GPU    | Target Architecture |
    | ----------- | ------------------- |
    | Nvidia K80  | `sm_35`             |
    | Nvidia P100 | `sm_60`             |
    | Nvidia V100 | `sm_70`             |
    | Nvidia A100 | `sm_80`             |

=== "AMD/ROCm"

    ```shell hl_lines="2-5"
    iree-compile \
        --iree-hal-target-backends=rocm \
        --iree-rocm-target-chip=<...> \
        --iree-rocm-link-bc=true \
        --iree-rocm-bc-dir=<...> \
        mobilenet_iree_input.mlir -o mobilenet_rocm.vmfb
    ```

    Note ROCm Bitcode Dir(`iree-rocm-bc-dir`) path is required. If the system
    you are compiling IREE in has ROCm installed, then the default value of
    `/opt/rocm/amdgcn/bitcode` will usually suffice. If you intend on building
    ROCm compiler in a non-ROCm capable system, please set `iree-rocm-bc-dir`
    to the absolute path where you might have saved the amdgcn bitcode.

    Note that a ROCm target chip(`iree-rocm-target-chip`) of the form
    `gfx<arch_number>` is needed to compile towards each GPU architecture. If
    no architecture is specified then we will default to `gfx908`.

    Here is a table of commonly used architectures:

    | AMD GPU   | Target Chip |
    | --------- | ----------- |
    | AMD MI25  | `gfx900`    |
    | AMD MI50  | `gfx906`    |
    | AMD MI60  | `gfx906`    |
    | AMD MI100 | `gfx908`    |

### :octicons-terminal-16: Run a compiled program

Run the following command:

=== "Nvidia/CUDA"

    ``` shell hl_lines="2"
    iree-run-module \
        --device=cuda \
        --module=mobilenet_cuda.vmfb \
        --function=predict \
        --input="1x224x224x3xf32=0"
    ```

=== "AMD/ROCm"

    ``` shell hl_lines="2"
    iree-run-module \
        --device=rocm \
        --module=mobilenet_rocm.vmfb \
        --function=predict \
        --input="1x224x224x3xf32=0"
    ```

The above assumes the exported function in the model is named as `predict` and
it expects one 224x224 RGB image. We are feeding in an image with all 0 values
here for brevity, see `iree-run-module --help` for the format to specify
concrete values.
