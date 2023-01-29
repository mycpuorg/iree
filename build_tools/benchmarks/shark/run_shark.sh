#!/bin/bash

# Copyright 2022 The IREE Authors
#
# Licensed under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Runs SHARK tank using both SHARK-Runtime and IREE-Runtime, producing benchmark
# numbers.
#
# Usage:
#    ./run_shark.sh \
#        --sha <SHA of https://github.com/nod-ai/SHARK.git to pin to> \
#        --pytest-regex e.g. "cpu", "cuda", "cuda and torch". \
#        --driver e.g. "cpu", "cuda", "vulkan" \
#        --output-dir <local output dir> \
#        --save-version-info <true|false>

set -xeuo pipefail

export SAVE_VERSION_INFO=false
export BENCHMARK_REGEX="cpu"
export DRIVER="cpu"

while [[ "$#" -gt 0 ]]; do
  case $1 in
    -s|--sha) SHARK_SHA="$2"
    shift;;
    -r|--pytest-regex) BENCHMARK_REGEX="$2"
    shift;;
    -d|--driver) DRIVER="$2"
    shift;;
    -o|--output-dir) SHARK_OUTPUT_DIR="$(pwd)/$2"
    shift;;
    -v|--save-version-info) SAVE_VERSION_INFO="$2"
    shift;;
    *) echo "Unknown parameter passed: $1"
    exit 1;;
esac
shift
done

mkdir "${SHARK_OUTPUT_DIR}"

git clone https://github.com/nod-ai/SHARK.git
cd SHARK
git reset --hard ${SHARK_SHA}

# Remove existing data.
rm -rf ./shark_tmp
rm -rf ~/.local/shark_tank

declare -a args=(
  --benchmark
  --update_tank
  --maxfail=500
  -k "${BENCHMARK_REGEX}"
)

if [[ ${DRIVER} == "cuda" ]]; then
  args+=(--tf32)
fi

# Run with SHARK-Runtime.
PYTHON=python3.10 VENV_DIR=shark.venv BENCHMARK=1 IMPORTER=1 ./setup_venv.sh
source shark.venv/bin/activate

export SHARK_VERSION=`pip show iree-compiler | grep Version | sed -e "s/^Version: \(.*\)$/\1/g"`
pytest "${args[@]}" tank/test_models.py || true

echo "######################################################"
echo "Benchmarks for SHARK-Runtime Complete"
cat bench_results.csv
mv bench_results.csv "${SHARK_OUTPUT_DIR}/${DRIVER}_shark_${SHARK_VERSION}.csv"
echo "######################################################"
deactivate

# Remove existing data.
rm -rf ./shark_tmp
rm -rf ~/.local/shark_tank

# Run with IREE.
PYTHON=python3.10 VENV_DIR=iree.venv BENCHMARK=1 IMPORTER=1 USE_IREE=1 ./setup_venv.sh
source iree.venv/bin/activate

export IREE_VERSION=`pip show iree-compiler | grep Version | sed -e "s/^Version: \(.*\)$/\1/g"`
pytest "${args[@]}" tank/test_models.py || true

echo "######################################################"
echo "Benchmarks for IREE Complete"
cat bench_results.csv
mv bench_results.csv "${SHARK_OUTPUT_DIR}/${DRIVER}_iree_${IREE_VERSION}.csv"
echo "######################################################"

# Save version info.
if [[ "${SAVE_VERSION_INFO}" == true ]]; then
  touch "${SHARK_OUTPUT_DIR}/version_info.txt"
  echo "iree=${IREE_VERSION}" >> "${SHARK_OUTPUT_DIR}/version_info.txt"
  echo "shark=${SHARK_VERSION}" >> "${SHARK_OUTPUT_DIR}/version_info.txt"

  export TF_VERSION=$(pip show tensorflow | grep "Version" | awk '{print $NF}')
  echo "tf=Tensorflow XLA ${TF_VERSION}" >> "${SHARK_OUTPUT_DIR}/version_info.txt"

  export TORCH_VERSION=$(pip show torch | grep "Version" | awk '{print $NF}')
  echo "torch=Torch Inductor ${TORCH_VERSION}" >> "${SHARK_OUTPUT_DIR}/version_info.txt"
fi

deactivate
