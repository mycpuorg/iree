#!/bin/bash

# Copyright 2022 The IREE Authors
#
# Licensed under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# This is a WIP script for creating managed instance groups. It isn't really a
# proper script, since most everything is hardcoded. Mostly it's a way to
# document the necessary commands. In due course, it may be turned into a proper
# script, probably in Python, but for now just edit the environment variables at
# the top.

set -euo pipefail

# For now, just change these parameters
VERSION=7138511883-62g-testing
REGION=us-west1
ZONES=us-west1-a,us-west1-b,us-west1-c
AUTOSCALING=1
GROUP=presubmit
TYPE=cpu
# For GPU groups, these should both be set to the target group size, as
# autoscaling currently does not work for these.
MIN_SIZE=3
MAX_SIZE=3
# Whether this is a testing MIG (i.e. not prod)
TESTING=1

function create_mig() {
  local runner_group="$1"
  local type="$2"

  local mig_name="github-runner"
  if (( TESTING == 1 )); then
    mig_name+="-testing"
  fi
  mig_name+="-${runner_group}-${type}-${REGION}"
  template="github-runner-${runner_group}-${type}-${VERSION}"

  local -a create_args=(
    "${mig_name}"
    --project=iree-oss
    --base-instance-name="${mig_name}"
    --size="${MIN_SIZE}"
    --template="${template}"
    --zones="${ZONES}"
    --region="${REGION}"
    --target-distribution-shape=EVEN
    --health-check=http-8080-ok
  )

  (set -x; gcloud beta compute instance-groups managed create "${create_args[@]}")
  echo ""

  local -a autoscaling_args=(
    "${mig_name}"
    --project=iree-oss
    --region="${REGION}"
    --cool-down-period=60
    --min-num-replicas="${MIN_SIZE}"
    --max-num-replicas="${MAX_SIZE}"
    --mode=only-scale-out
    --target-cpu-utilization=0.6
  )

  (set -x; gcloud beta compute instance-groups managed set-autoscaling "${autoscaling_args[@]}")
  echo ""
}

create_mig "${GROUP}" "${TYPE}"
