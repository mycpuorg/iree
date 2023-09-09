// Copyright 2022 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "iree/builtins/ukernel/arch/arm_64/common_arm_64.h"
#include "iree/builtins/ukernel/arch/arm_64/mmt4d_arm_64_internal.h"

void iree_uk_mmt4d_tile_f32f32f32_8x8x1_arm_64(
    void* IREE_UK_RESTRICT out_tile, const void* IREE_UK_RESTRICT lhs_panel,
    const void* IREE_UK_RESTRICT rhs_panel,
    const iree_uk_mmt4d_params_t* params) {
  const float* IREE_UK_RESTRICT lhs_ptr = lhs_panel;
  const float* IREE_UK_RESTRICT rhs_ptr = rhs_panel;
  float* IREE_UK_RESTRICT out_ptr = out_tile;
  float32x4_t acc0, acc1, acc2, acc3, acc4, acc5, acc6, acc7, acc8, acc9, acc10,
      acc11, acc12, acc13, acc14, acc15;
  if (params->flags & IREE_UK_FLAG_MMT4D_ACCUMULATE) {
    acc0 = vld1q_f32(out_ptr + 4 * 0);
    acc1 = vld1q_f32(out_ptr + 4 * 1);
    acc2 = vld1q_f32(out_ptr + 4 * 2);
    acc3 = vld1q_f32(out_ptr + 4 * 3);
    acc4 = vld1q_f32(out_ptr + 4 * 4);
    acc5 = vld1q_f32(out_ptr + 4 * 5);
    acc6 = vld1q_f32(out_ptr + 4 * 6);
    acc7 = vld1q_f32(out_ptr + 4 * 7);
    acc8 = vld1q_f32(out_ptr + 4 * 8);
    acc9 = vld1q_f32(out_ptr + 4 * 9);
    acc10 = vld1q_f32(out_ptr + 4 * 10);
    acc11 = vld1q_f32(out_ptr + 4 * 11);
    acc12 = vld1q_f32(out_ptr + 4 * 12);
    acc13 = vld1q_f32(out_ptr + 4 * 13);
    acc14 = vld1q_f32(out_ptr + 4 * 14);
    acc15 = vld1q_f32(out_ptr + 4 * 15);
  } else {
    acc0 = vdupq_n_f32(0);
    acc1 = vdupq_n_f32(0);
    acc2 = vdupq_n_f32(0);
    acc3 = vdupq_n_f32(0);
    acc4 = vdupq_n_f32(0);
    acc5 = vdupq_n_f32(0);
    acc6 = vdupq_n_f32(0);
    acc7 = vdupq_n_f32(0);
    acc8 = vdupq_n_f32(0);
    acc9 = vdupq_n_f32(0);
    acc10 = vdupq_n_f32(0);
    acc11 = vdupq_n_f32(0);
    acc12 = vdupq_n_f32(0);
    acc13 = vdupq_n_f32(0);
    acc14 = vdupq_n_f32(0);
    acc15 = vdupq_n_f32(0);
  }
  IREE_UK_ASSUME(params->K >= 1);
  for (int k = 0; k < params->K; ++k) {
    float32x4_t lhs0 = vld1q_f32(lhs_ptr + 0);
    float32x4_t lhs1 = vld1q_f32(lhs_ptr + 4);
    lhs_ptr += 8;
    float32x4_t rhs0 = vld1q_f32(rhs_ptr + 0);
    float32x4_t rhs1 = vld1q_f32(rhs_ptr + 4);
    rhs_ptr += 8;
    acc0 = vfmaq_lane_f32(acc0, rhs0, vget_low_f32(lhs0), 0);
    acc1 = vfmaq_lane_f32(acc1, rhs1, vget_low_f32(lhs0), 0);
    acc2 = vfmaq_lane_f32(acc2, rhs0, vget_low_f32(lhs0), 1);
    acc3 = vfmaq_lane_f32(acc3, rhs1, vget_low_f32(lhs0), 1);
    acc4 = vfmaq_lane_f32(acc4, rhs0, vget_high_f32(lhs0), 0);
    acc5 = vfmaq_lane_f32(acc5, rhs1, vget_high_f32(lhs0), 0);
    acc6 = vfmaq_lane_f32(acc6, rhs0, vget_high_f32(lhs0), 1);
    acc7 = vfmaq_lane_f32(acc7, rhs1, vget_high_f32(lhs0), 1);
    acc8 = vfmaq_lane_f32(acc8, rhs0, vget_low_f32(lhs1), 0);
    acc9 = vfmaq_lane_f32(acc9, rhs1, vget_low_f32(lhs1), 0);
    acc10 = vfmaq_lane_f32(acc10, rhs0, vget_low_f32(lhs1), 1);
    acc11 = vfmaq_lane_f32(acc11, rhs1, vget_low_f32(lhs1), 1);
    acc12 = vfmaq_lane_f32(acc12, rhs0, vget_high_f32(lhs1), 0);
    acc13 = vfmaq_lane_f32(acc13, rhs1, vget_high_f32(lhs1), 0);
    acc14 = vfmaq_lane_f32(acc14, rhs0, vget_high_f32(lhs1), 1);
    acc15 = vfmaq_lane_f32(acc15, rhs1, vget_high_f32(lhs1), 1);
  }
  vst1q_f32(out_ptr + 4 * 0, acc0);
  vst1q_f32(out_ptr + 4 * 1, acc1);
  vst1q_f32(out_ptr + 4 * 2, acc2);
  vst1q_f32(out_ptr + 4 * 3, acc3);
  vst1q_f32(out_ptr + 4 * 4, acc4);
  vst1q_f32(out_ptr + 4 * 5, acc5);
  vst1q_f32(out_ptr + 4 * 6, acc6);
  vst1q_f32(out_ptr + 4 * 7, acc7);
  vst1q_f32(out_ptr + 4 * 8, acc8);
  vst1q_f32(out_ptr + 4 * 9, acc9);
  vst1q_f32(out_ptr + 4 * 10, acc10);
  vst1q_f32(out_ptr + 4 * 11, acc11);
  vst1q_f32(out_ptr + 4 * 12, acc12);
  vst1q_f32(out_ptr + 4 * 13, acc13);
  vst1q_f32(out_ptr + 4 * 14, acc14);
  vst1q_f32(out_ptr + 4 * 15, acc15);
}

// Shared implementation for f16f16f16 and f16f16f32.
// In the f16f16f16 case, intermediate roundings are skipped. This function
// should only be used if IREE_UK_FLAG_MMT4D_SKIP_INTERMEDIATE_ROUNDINGS is set.
static void iree_uk_mmt4d_tile_f16f16fXX_8x8x1_arm_64(
    void* IREE_UK_RESTRICT out_tile, const void* IREE_UK_RESTRICT lhs_panel,
    const void* IREE_UK_RESTRICT rhs_panel,
    const iree_uk_mmt4d_params_t* params, iree_uk_type_t acc_type) {
  const float16_t* IREE_UK_RESTRICT lhs_ptr = lhs_panel;
  const float16_t* IREE_UK_RESTRICT rhs_ptr = rhs_panel;
  float32x4_t acc0, acc1, acc2, acc3, acc4, acc5, acc6, acc7, acc8, acc9, acc10,
      acc11, acc12, acc13, acc14, acc15;
  if (params->flags & IREE_UK_FLAG_MMT4D_ACCUMULATE) {
    if (acc_type == IREE_UK_TYPE_FLOAT_32) {
      float* IREE_UK_RESTRICT out_ptr = out_tile;
      acc0 = vld1q_f32(out_ptr + 4 * 0);
      acc1 = vld1q_f32(out_ptr + 4 * 1);
      acc2 = vld1q_f32(out_ptr + 4 * 2);
      acc3 = vld1q_f32(out_ptr + 4 * 3);
      acc4 = vld1q_f32(out_ptr + 4 * 4);
      acc5 = vld1q_f32(out_ptr + 4 * 5);
      acc6 = vld1q_f32(out_ptr + 4 * 6);
      acc7 = vld1q_f32(out_ptr + 4 * 7);
      acc8 = vld1q_f32(out_ptr + 4 * 8);
      acc9 = vld1q_f32(out_ptr + 4 * 9);
      acc10 = vld1q_f32(out_ptr + 4 * 10);
      acc11 = vld1q_f32(out_ptr + 4 * 11);
      acc12 = vld1q_f32(out_ptr + 4 * 12);
      acc13 = vld1q_f32(out_ptr + 4 * 13);
      acc14 = vld1q_f32(out_ptr + 4 * 14);
      acc15 = vld1q_f32(out_ptr + 4 * 15);
    } else {
      float16_t* IREE_UK_RESTRICT out_ptr = out_tile;
      acc0 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 0));
      acc1 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 1));
      acc2 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 2));
      acc3 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 3));
      acc4 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 4));
      acc5 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 5));
      acc6 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 6));
      acc7 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 7));
      acc8 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 8));
      acc9 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 9));
      acc10 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 10));
      acc11 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 11));
      acc12 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 12));
      acc13 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 13));
      acc14 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 14));
      acc15 = vcvt_f32_f16(vld1_f16(out_ptr + 4 * 15));
    }
  } else {
    acc0 = vdupq_n_f32(0);
    acc1 = vdupq_n_f32(0);
    acc2 = vdupq_n_f32(0);
    acc3 = vdupq_n_f32(0);
    acc4 = vdupq_n_f32(0);
    acc5 = vdupq_n_f32(0);
    acc6 = vdupq_n_f32(0);
    acc7 = vdupq_n_f32(0);
    acc8 = vdupq_n_f32(0);
    acc9 = vdupq_n_f32(0);
    acc10 = vdupq_n_f32(0);
    acc11 = vdupq_n_f32(0);
    acc12 = vdupq_n_f32(0);
    acc13 = vdupq_n_f32(0);
    acc14 = vdupq_n_f32(0);
    acc15 = vdupq_n_f32(0);
  }
  IREE_UK_ASSUME(params->K >= 1);
  for (int k = 0; k < params->K; ++k) {
    float32x4_t lhs0 = vcvt_f32_f16(vld1_f16(lhs_ptr + 0));
    float32x4_t lhs1 = vcvt_f32_f16(vld1_f16(lhs_ptr + 4));
    lhs_ptr += 8;
    float32x4_t rhs0 = vcvt_f32_f16(vld1_f16(rhs_ptr + 0));
    float32x4_t rhs1 = vcvt_f32_f16(vld1_f16(rhs_ptr + 4));
    rhs_ptr += 8;
    acc0 = vfmaq_lane_f32(acc0, rhs0, vget_low_f32(lhs0), 0);
    acc1 = vfmaq_lane_f32(acc1, rhs1, vget_low_f32(lhs0), 0);
    acc2 = vfmaq_lane_f32(acc2, rhs0, vget_low_f32(lhs0), 1);
    acc3 = vfmaq_lane_f32(acc3, rhs1, vget_low_f32(lhs0), 1);
    acc4 = vfmaq_lane_f32(acc4, rhs0, vget_high_f32(lhs0), 0);
    acc5 = vfmaq_lane_f32(acc5, rhs1, vget_high_f32(lhs0), 0);
    acc6 = vfmaq_lane_f32(acc6, rhs0, vget_high_f32(lhs0), 1);
    acc7 = vfmaq_lane_f32(acc7, rhs1, vget_high_f32(lhs0), 1);
    acc8 = vfmaq_lane_f32(acc8, rhs0, vget_low_f32(lhs1), 0);
    acc9 = vfmaq_lane_f32(acc9, rhs1, vget_low_f32(lhs1), 0);
    acc10 = vfmaq_lane_f32(acc10, rhs0, vget_low_f32(lhs1), 1);
    acc11 = vfmaq_lane_f32(acc11, rhs1, vget_low_f32(lhs1), 1);
    acc12 = vfmaq_lane_f32(acc12, rhs0, vget_high_f32(lhs1), 0);
    acc13 = vfmaq_lane_f32(acc13, rhs1, vget_high_f32(lhs1), 0);
    acc14 = vfmaq_lane_f32(acc14, rhs0, vget_high_f32(lhs1), 1);
    acc15 = vfmaq_lane_f32(acc15, rhs1, vget_high_f32(lhs1), 1);
  }
  if (acc_type == IREE_UK_TYPE_FLOAT_32) {
    float* IREE_UK_RESTRICT out_ptr = out_tile;
    vst1q_f32(out_ptr + 4 * 0, acc0);
    vst1q_f32(out_ptr + 4 * 1, acc1);
    vst1q_f32(out_ptr + 4 * 2, acc2);
    vst1q_f32(out_ptr + 4 * 3, acc3);
    vst1q_f32(out_ptr + 4 * 4, acc4);
    vst1q_f32(out_ptr + 4 * 5, acc5);
    vst1q_f32(out_ptr + 4 * 6, acc6);
    vst1q_f32(out_ptr + 4 * 7, acc7);
    vst1q_f32(out_ptr + 4 * 8, acc8);
    vst1q_f32(out_ptr + 4 * 9, acc9);
    vst1q_f32(out_ptr + 4 * 10, acc10);
    vst1q_f32(out_ptr + 4 * 11, acc11);
    vst1q_f32(out_ptr + 4 * 12, acc12);
    vst1q_f32(out_ptr + 4 * 13, acc13);
    vst1q_f32(out_ptr + 4 * 14, acc14);
    vst1q_f32(out_ptr + 4 * 15, acc15);
  } else {
    float16_t* IREE_UK_RESTRICT out_ptr = out_tile;
    vst1_f16(out_ptr + 4 * 0, vcvt_f16_f32(acc0));
    vst1_f16(out_ptr + 4 * 1, vcvt_f16_f32(acc1));
    vst1_f16(out_ptr + 4 * 2, vcvt_f16_f32(acc2));
    vst1_f16(out_ptr + 4 * 3, vcvt_f16_f32(acc3));
    vst1_f16(out_ptr + 4 * 4, vcvt_f16_f32(acc4));
    vst1_f16(out_ptr + 4 * 5, vcvt_f16_f32(acc5));
    vst1_f16(out_ptr + 4 * 6, vcvt_f16_f32(acc6));
    vst1_f16(out_ptr + 4 * 7, vcvt_f16_f32(acc7));
    vst1_f16(out_ptr + 4 * 8, vcvt_f16_f32(acc8));
    vst1_f16(out_ptr + 4 * 9, vcvt_f16_f32(acc9));
    vst1_f16(out_ptr + 4 * 10, vcvt_f16_f32(acc10));
    vst1_f16(out_ptr + 4 * 11, vcvt_f16_f32(acc11));
    vst1_f16(out_ptr + 4 * 12, vcvt_f16_f32(acc12));
    vst1_f16(out_ptr + 4 * 13, vcvt_f16_f32(acc13));
    vst1_f16(out_ptr + 4 * 14, vcvt_f16_f32(acc14));
    vst1_f16(out_ptr + 4 * 15, vcvt_f16_f32(acc15));
  }
}

void iree_uk_mmt4d_tile_f16f16f16_8x8x1_arm_64(
    void* IREE_UK_RESTRICT out_tile, const void* IREE_UK_RESTRICT lhs_panel,
    const void* IREE_UK_RESTRICT rhs_panel,
    const iree_uk_mmt4d_params_t* params) {
  iree_uk_mmt4d_tile_f16f16fXX_8x8x1_arm_64(out_tile, lhs_panel, rhs_panel,
                                            params, IREE_UK_TYPE_FLOAT_16);
}

void iree_uk_mmt4d_tile_f16f16f32_8x8x1_arm_64(
    void* IREE_UK_RESTRICT out_tile, const void* IREE_UK_RESTRICT lhs_panel,
    const void* IREE_UK_RESTRICT rhs_panel,
    const iree_uk_mmt4d_params_t* params) {
  iree_uk_mmt4d_tile_f16f16fXX_8x8x1_arm_64(out_tile, lhs_panel, rhs_panel,
                                            params, IREE_UK_TYPE_FLOAT_32);
}

void iree_uk_mmt4d_tile_i8i8i32_8x8x1_arm_64(
    void* IREE_UK_RESTRICT out_tile, const void* IREE_UK_RESTRICT lhs_panel,
    const void* IREE_UK_RESTRICT rhs_panel,
    const iree_uk_mmt4d_params_t* params) {
  const iree_uk_int8_t* IREE_UK_RESTRICT lhs_ptr = lhs_panel;
  const iree_uk_int8_t* IREE_UK_RESTRICT rhs_ptr = rhs_panel;
  iree_uk_int32_t* IREE_UK_RESTRICT out_ptr = out_tile;
  int32x4_t acc0, acc1, acc2, acc3, acc4, acc5, acc6, acc7, acc8, acc9, acc10,
      acc11, acc12, acc13, acc14, acc15;
  if (params->flags & IREE_UK_FLAG_MMT4D_ACCUMULATE) {
    acc0 = vld1q_s32(out_ptr + 4 * 0);
    acc1 = vld1q_s32(out_ptr + 4 * 1);
    acc2 = vld1q_s32(out_ptr + 4 * 2);
    acc3 = vld1q_s32(out_ptr + 4 * 3);
    acc4 = vld1q_s32(out_ptr + 4 * 4);
    acc5 = vld1q_s32(out_ptr + 4 * 5);
    acc6 = vld1q_s32(out_ptr + 4 * 6);
    acc7 = vld1q_s32(out_ptr + 4 * 7);
    acc8 = vld1q_s32(out_ptr + 4 * 8);
    acc9 = vld1q_s32(out_ptr + 4 * 9);
    acc10 = vld1q_s32(out_ptr + 4 * 10);
    acc11 = vld1q_s32(out_ptr + 4 * 11);
    acc12 = vld1q_s32(out_ptr + 4 * 12);
    acc13 = vld1q_s32(out_ptr + 4 * 13);
    acc14 = vld1q_s32(out_ptr + 4 * 14);
    acc15 = vld1q_s32(out_ptr + 4 * 15);
  } else {
    acc0 = vdupq_n_s32(0);
    acc1 = vdupq_n_s32(0);
    acc2 = vdupq_n_s32(0);
    acc3 = vdupq_n_s32(0);
    acc4 = vdupq_n_s32(0);
    acc5 = vdupq_n_s32(0);
    acc6 = vdupq_n_s32(0);
    acc7 = vdupq_n_s32(0);
    acc8 = vdupq_n_s32(0);
    acc9 = vdupq_n_s32(0);
    acc10 = vdupq_n_s32(0);
    acc11 = vdupq_n_s32(0);
    acc12 = vdupq_n_s32(0);
    acc13 = vdupq_n_s32(0);
    acc14 = vdupq_n_s32(0);
    acc15 = vdupq_n_s32(0);
  }
  IREE_UK_ASSUME(params->K >= 1);
  for (int k = 0; k < params->K; ++k) {
    int16x8_t lhs = vmovl_s8(vld1_s8(lhs_ptr));
    lhs_ptr += 8;
    int16x8_t rhs = vmovl_s8(vld1_s8(rhs_ptr));
    rhs_ptr += 8;
    acc0 = vmlal_lane_s16(acc0, vget_low_s16(rhs), vget_low_s16(lhs), 0);
    acc1 = vmlal_lane_s16(acc1, vget_high_s16(rhs), vget_low_s16(lhs), 0);
    acc2 = vmlal_lane_s16(acc2, vget_low_s16(rhs), vget_low_s16(lhs), 1);
    acc3 = vmlal_lane_s16(acc3, vget_high_s16(rhs), vget_low_s16(lhs), 1);
    acc4 = vmlal_lane_s16(acc4, vget_low_s16(rhs), vget_low_s16(lhs), 2);
    acc5 = vmlal_lane_s16(acc5, vget_high_s16(rhs), vget_low_s16(lhs), 2);
    acc6 = vmlal_lane_s16(acc6, vget_low_s16(rhs), vget_low_s16(lhs), 3);
    acc7 = vmlal_lane_s16(acc7, vget_high_s16(rhs), vget_low_s16(lhs), 3);
    acc8 = vmlal_lane_s16(acc8, vget_low_s16(rhs), vget_high_s16(lhs), 0);
    acc9 = vmlal_lane_s16(acc9, vget_high_s16(rhs), vget_high_s16(lhs), 0);
    acc10 = vmlal_lane_s16(acc10, vget_low_s16(rhs), vget_high_s16(lhs), 1);
    acc11 = vmlal_lane_s16(acc11, vget_high_s16(rhs), vget_high_s16(lhs), 1);
    acc12 = vmlal_lane_s16(acc12, vget_low_s16(rhs), vget_high_s16(lhs), 2);
    acc13 = vmlal_lane_s16(acc13, vget_high_s16(rhs), vget_high_s16(lhs), 2);
    acc14 = vmlal_lane_s16(acc14, vget_low_s16(rhs), vget_high_s16(lhs), 3);
    acc15 = vmlal_lane_s16(acc15, vget_high_s16(rhs), vget_high_s16(lhs), 3);
  }
  vst1q_s32(out_ptr + 4 * 0, acc0);
  vst1q_s32(out_ptr + 4 * 1, acc1);
  vst1q_s32(out_ptr + 4 * 2, acc2);
  vst1q_s32(out_ptr + 4 * 3, acc3);
  vst1q_s32(out_ptr + 4 * 4, acc4);
  vst1q_s32(out_ptr + 4 * 5, acc5);
  vst1q_s32(out_ptr + 4 * 6, acc6);
  vst1q_s32(out_ptr + 4 * 7, acc7);
  vst1q_s32(out_ptr + 4 * 8, acc8);
  vst1q_s32(out_ptr + 4 * 9, acc9);
  vst1q_s32(out_ptr + 4 * 10, acc10);
  vst1q_s32(out_ptr + 4 * 11, acc11);
  vst1q_s32(out_ptr + 4 * 12, acc12);
  vst1q_s32(out_ptr + 4 * 13, acc13);
  vst1q_s32(out_ptr + 4 * 14, acc14);
  vst1q_s32(out_ptr + 4 * 15, acc15);
}
