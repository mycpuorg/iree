// Copyright 2023 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "iree/builtins/ukernel/arch/x86_64/common_x86_64_entry_point.h"
#include "iree/builtins/ukernel/arch/x86_64/mmt4d_x86_64_internal.h"

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_f32f32f32_8x8x1(
    const iree_uk_mmt4d_params_t* params) {
#if defined(IREE_UK_BUILD_X86_64_AVX2_FMA)
  if (iree_uk_cpu_supports_avx2_fma(params->cpu_data)) {
    return iree_uk_mmt4d_tile_f32f32f32_8x8x1_x86_64_avx2_fma;
  }
#endif
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_f32f32f32_16x16x1(
    const iree_uk_mmt4d_params_t* params) {
#if defined(IREE_UK_BUILD_X86_64_AVX512_BASE)
  if (iree_uk_cpu_supports_avx512_base(params->cpu_data)) {
    return iree_uk_mmt4d_tile_f32f32f32_16x16x1_x86_64_avx512_base;
  }
#endif
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_f16f16f32_8x8x1(
    const iree_uk_mmt4d_params_t* params) {
#if defined(IREE_UK_BUILD_X86_64_AVX2_FMA)
  if (iree_uk_cpu_supports_avx2_fma(params->cpu_data)) {
    return iree_uk_mmt4d_tile_f16f16f32_8x8x1_x86_64_avx2_fma;
  }
#endif
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_f16f16f32_16x16x1(
    const iree_uk_mmt4d_params_t* params) {
#if defined(IREE_UK_BUILD_X86_64_AVX512_BASE)
  if (iree_uk_cpu_supports_avx512_base(params->cpu_data)) {
    return iree_uk_mmt4d_tile_f16f16f32_16x16x1_x86_64_avx512_base;
  }
#endif
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_f16f16f16_8x8x1(
    const iree_uk_mmt4d_params_t* params) {
#if defined(IREE_UK_BUILD_X86_64_AVX2_FMA)
  if ((params->flags & IREE_UK_FLAG_MMT4D_SKIP_INTERMEDIATE_ROUNDINGS) &&
      iree_uk_cpu_supports_avx2_fma(params->cpu_data)) {
    return iree_uk_mmt4d_tile_f16f16f16_8x8x1_x86_64_avx2_fma;
  }
#endif
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_f16f16f16_16x16x1(
    const iree_uk_mmt4d_params_t* params) {
#if defined(IREE_UK_BUILD_X86_64_AVX512_BASE)
  if ((params->flags & IREE_UK_FLAG_MMT4D_SKIP_INTERMEDIATE_ROUNDINGS) &&
      iree_uk_cpu_supports_avx512_base(params->cpu_data)) {
    return iree_uk_mmt4d_tile_f16f16f16_16x16x1_x86_64_avx512_base;
  }
#endif
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_i8i8i32_16x16x2(
    const iree_uk_mmt4d_params_t* params) {
#if defined(IREE_UK_BUILD_X86_64_AVX512_VNNI)
  if (params->cpu_data[0] & (IREE_CPU_DATA0_X86_64_AVX512VNNI)) {
    return iree_uk_mmt4d_tile_i8i8i32_16x16x2_x86_64_avx512_vnni;
  }
#endif
#if defined(IREE_UK_BUILD_X86_64_AVX512_BASE)
  if (params->cpu_data[0] & (IREE_CPU_DATA0_X86_64_AVX512BW)) {
    return iree_uk_mmt4d_tile_i8i8i32_16x16x2_x86_64_avx512_base;
  }
#endif
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_i8i8i32_8x8x2(
    const iree_uk_mmt4d_params_t* params) {
#if defined(IREE_UK_BUILD_X86_64_AVX2_FMA)
  if (iree_uk_cpu_supports_avx2_fma(params->cpu_data)) {
    return iree_uk_mmt4d_tile_i8i8i32_8x8x2_x86_64_avx2_fma;
  }
#endif
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_f32f32f32(
    const iree_uk_mmt4d_params_t* params) {
  if (params->M0 == 16 && params->N0 == 16 && params->K0 == 1) {
    return iree_uk_mmt4d_select_tile_func_x86_64_f32f32f32_16x16x1(params);
  }
  if (params->M0 == 8 && params->N0 == 8 && params->K0 == 1) {
    return iree_uk_mmt4d_select_tile_func_x86_64_f32f32f32_8x8x1(params);
  }
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_f16f16f32(
    const iree_uk_mmt4d_params_t* params) {
  if (params->M0 == 16 && params->N0 == 16 && params->K0 == 1) {
    return iree_uk_mmt4d_select_tile_func_x86_64_f16f16f32_16x16x1(params);
  }
  if (params->M0 == 8 && params->N0 == 8 && params->K0 == 1) {
    return iree_uk_mmt4d_select_tile_func_x86_64_f16f16f32_8x8x1(params);
  }
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_f16f16f16(
    const iree_uk_mmt4d_params_t* params) {
  if (params->M0 == 16 && params->N0 == 16 && params->K0 == 1) {
    return iree_uk_mmt4d_select_tile_func_x86_64_f16f16f16_16x16x1(params);
  }
  if (params->M0 == 8 && params->N0 == 8 && params->K0 == 1) {
    return iree_uk_mmt4d_select_tile_func_x86_64_f16f16f16_8x8x1(params);
  }
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_bf16bf16f32(
    const iree_uk_mmt4d_params_t* params) {
  return 0;
}

static iree_uk_mmt4d_tile_func_t
iree_uk_mmt4d_select_tile_func_x86_64_bf16bf16bf16(
    const iree_uk_mmt4d_params_t* params) {
  return 0;
}

static iree_uk_mmt4d_tile_func_t iree_uk_mmt4d_select_tile_func_x86_64_i8i8i32(
    const iree_uk_mmt4d_params_t* params) {
  if (params->M0 == 16 && params->N0 == 16 && params->K0 == 2) {
    return iree_uk_mmt4d_select_tile_func_x86_64_i8i8i32_16x16x2(params);
  }
  if (params->M0 == 8 && params->N0 == 8 && params->K0 == 2) {
    return iree_uk_mmt4d_select_tile_func_x86_64_i8i8i32_8x8x2(params);
  }
  return 0;
}

iree_uk_mmt4d_tile_func_t iree_uk_mmt4d_select_tile_func_arch(
    const iree_uk_mmt4d_params_t* params) {
  switch (iree_uk_mmt4d_type(params->flags)) {
    case iree_uk_mmt4d_type_f32f32f32:
      return iree_uk_mmt4d_select_tile_func_x86_64_f32f32f32(params);
    case iree_uk_mmt4d_type_f16f16f32:
      return iree_uk_mmt4d_select_tile_func_x86_64_f16f16f32(params);
    case iree_uk_mmt4d_type_f16f16f16:
      return iree_uk_mmt4d_select_tile_func_x86_64_f16f16f16(params);
    case iree_uk_mmt4d_type_bf16bf16f32:
      return iree_uk_mmt4d_select_tile_func_x86_64_bf16bf16f32(params);
    case iree_uk_mmt4d_type_bf16bf16bf16:
      return iree_uk_mmt4d_select_tile_func_x86_64_bf16bf16bf16(params);
    case iree_uk_mmt4d_type_i8i8i32:
      return iree_uk_mmt4d_select_tile_func_x86_64_i8i8i32(params);
    default:
      IREE_UK_ASSUME_UNREACHABLE;
      return 0;
  }
}
