# Copyright 2022 The IREE Authors
#
# Licensed under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# An image for running mmperf: https://github.com/mmperf/mmperf.
#
# mmperf benchmarks matrix-multiplication workloads on IREE and various backends
# such as OpenBLAS, MKL, TVM, Halide, CuBLAS, etc.
#
# These backends are included either in this image or as a submodule in the
# mmperf repo. Later versions of Clang, LLVM, Python and Ubuntu are needed
# to satisfy the dependency requirements of the backends.

FROM gcr.io/iree-oss/perf@sha256:0e1c92dde6ec7312c7c92a8ee329892ad83bae4b7968e4f2c98f99ffad3faa48

######## CUDA ########
RUN apt-get update \
  && apt-get install -y \
    nvidia-cuda-toolkit \
  && mkdir -p "/usr/nvvm/libdevice" \
  && ln -s "/usr/lib/nvidia-cuda-toolkit/libdevice/libdevice.10.bc" "/usr/nvvm/libdevice/libdevice.10.bc"
##############

######## MKL ########
WORKDIR /install-mkl

RUN wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS-2019.PUB \
    && apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS-2019.PUB \
    && sh -c 'echo deb https://apt.repos.intel.com/mkl all main > /etc/apt/sources.list.d/intel-mkl.list' \
    && apt-get update \
    && apt-get install -y intel-mkl-64bit-2018.2-046 \
    && rm -rf /install-mkl

WORKDIR /

ENV MKL_DIR="/opt/intel/mkl"
##############

######## OPENBLAS ########
RUN apt-get update \
  && apt-get install -y libopenblas-dev
##############

######## BLIS ########
WORKDIR /install-blis

RUN git clone --recurse-submodules https://github.com/flame/blis \
  && cd blis \
  && ./configure --prefix=/opt/blis --enable-cblas -c amd64 \
  && make -j 32 \
  && make install \
  && rm -rf /install-blis

WORKDIR /

ENV BLIS_DIR="/opt/blis"
##############

######## MMPERF ########
COPY build_tools/docker/context/setup_mmperf.sh /usr/local/bin

ARG MMPERF_SHA="1ebd6b9a72745e88b855c44d7320a92c83508f80"

# Generate a version of mmperf for CPU.
RUN mkdir -p "/usr/local/src/mmperf" \
    && /usr/local/bin/setup_mmperf.sh "/usr/local/src/mmperf" "${MMPERF_SHA}"

ENV MMPERF_REPO_DIR="/usr/local/src/mmperf/mmperf"
############## \
