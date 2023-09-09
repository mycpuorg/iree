// Copyright 2023 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "iree/builtins/ukernel/arch/x86_64/common_x86_64.h"
#include "iree/builtins/ukernel/arch/x86_64/mmt4d_x86_64_internal.h"

void iree_uk_mmt4d_tile_i8i8i32_16x16x2_x86_64_avx512_vnni(
    void* IREE_UK_RESTRICT out_tile, const void* IREE_UK_RESTRICT lhs_panel,
    const void* IREE_UK_RESTRICT rhs_panel,
    const iree_uk_mmt4d_params_t* params) {
  iree_uk_int32_t* IREE_UK_RESTRICT out_ptr = out_tile;
  const iree_uk_int8_t* IREE_UK_RESTRICT lhs_ptr = lhs_panel;
  const iree_uk_int8_t* IREE_UK_RESTRICT rhs_ptr = rhs_panel;

  __m512i acc_0_0123_4_4567_8_89AB_C_CDEF;
  __m512i acc_0_4567_4_0123_8_CDEF_C_89AB;
  __m512i acc_0_89AB_4_CDEF_8_0123_C_4567;
  __m512i acc_0_CDEF_4_89AB_8_4567_C_0123;
  __m512i acc_1_0123_5_4567_9_89AB_D_CDEF;
  __m512i acc_1_4567_5_0123_9_CDEF_D_89AB;
  __m512i acc_1_89AB_5_CDEF_9_0123_D_4567;
  __m512i acc_1_CDEF_5_89AB_9_4567_D_0123;
  __m512i acc_2_0123_6_4567_A_89AB_E_CDEF;
  __m512i acc_2_4567_6_0123_A_CDEF_E_89AB;
  __m512i acc_2_89AB_6_CDEF_A_0123_E_4567;
  __m512i acc_2_CDEF_6_89AB_A_4567_E_0123;
  __m512i acc_3_0123_7_4567_B_89AB_F_CDEF;
  __m512i acc_3_4567_7_0123_B_CDEF_F_89AB;
  __m512i acc_3_89AB_7_CDEF_B_0123_F_4567;
  __m512i acc_3_CDEF_7_89AB_B_4567_F_0123;

  if (params->flags & IREE_UK_FLAG_MMT4D_ACCUMULATE) {
    acc_0_0123_4_4567_8_89AB_C_CDEF = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 0, 0, 4, 4, 8, 8, 12, 12);
    acc_0_4567_4_0123_8_CDEF_C_89AB = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 0, 4, 4, 0, 8, 12, 12, 8);
    acc_0_89AB_4_CDEF_8_0123_C_4567 = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 0, 8, 4, 12, 8, 0, 12, 4);
    acc_0_CDEF_4_89AB_8_4567_C_0123 = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 0, 12, 4, 8, 8, 4, 12, 0);
    acc_1_0123_5_4567_9_89AB_D_CDEF = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 1, 0, 5, 4, 9, 8, 13, 12);
    acc_1_4567_5_0123_9_CDEF_D_89AB = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 1, 4, 5, 0, 9, 12, 13, 8);
    acc_1_89AB_5_CDEF_9_0123_D_4567 = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 1, 8, 5, 12, 9, 0, 13, 4);
    acc_1_CDEF_5_89AB_9_4567_D_0123 = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 1, 12, 5, 8, 9, 4, 13, 0);
    acc_2_0123_6_4567_A_89AB_E_CDEF = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 2, 0, 6, 4, 10, 8, 14, 12);
    acc_2_4567_6_0123_A_CDEF_E_89AB = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 2, 4, 6, 0, 10, 12, 14, 8);
    acc_2_89AB_6_CDEF_A_0123_E_4567 = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 2, 8, 6, 12, 10, 0, 14, 4);
    acc_2_CDEF_6_89AB_A_4567_E_0123 = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 2, 12, 6, 8, 10, 4, 14, 0);
    acc_3_0123_7_4567_B_89AB_F_CDEF = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 3, 0, 7, 4, 11, 8, 15, 12);
    acc_3_4567_7_0123_B_CDEF_F_89AB = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 3, 4, 7, 0, 11, 12, 15, 8);
    acc_3_89AB_7_CDEF_B_0123_F_4567 = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 3, 8, 7, 12, 11, 0, 15, 4);
    acc_3_CDEF_7_89AB_B_4567_F_0123 = iree_uk_avx512_loadu_4x128_from_16x16xi32(
        out_ptr, 3, 12, 7, 8, 11, 4, 15, 0);
  } else {
    acc_0_0123_4_4567_8_89AB_C_CDEF = _mm512_setzero_si512();
    acc_0_4567_4_0123_8_CDEF_C_89AB = _mm512_setzero_si512();
    acc_0_89AB_4_CDEF_8_0123_C_4567 = _mm512_setzero_si512();
    acc_0_CDEF_4_89AB_8_4567_C_0123 = _mm512_setzero_si512();
    acc_1_0123_5_4567_9_89AB_D_CDEF = _mm512_setzero_si512();
    acc_1_4567_5_0123_9_CDEF_D_89AB = _mm512_setzero_si512();
    acc_1_89AB_5_CDEF_9_0123_D_4567 = _mm512_setzero_si512();
    acc_1_CDEF_5_89AB_9_4567_D_0123 = _mm512_setzero_si512();
    acc_2_0123_6_4567_A_89AB_E_CDEF = _mm512_setzero_si512();
    acc_2_4567_6_0123_A_CDEF_E_89AB = _mm512_setzero_si512();
    acc_2_89AB_6_CDEF_A_0123_E_4567 = _mm512_setzero_si512();
    acc_2_CDEF_6_89AB_A_4567_E_0123 = _mm512_setzero_si512();
    acc_3_0123_7_4567_B_89AB_F_CDEF = _mm512_setzero_si512();
    acc_3_4567_7_0123_B_CDEF_F_89AB = _mm512_setzero_si512();
    acc_3_89AB_7_CDEF_B_0123_F_4567 = _mm512_setzero_si512();
    acc_3_CDEF_7_89AB_B_4567_F_0123 = _mm512_setzero_si512();
  }

  __m512i idx_45670123CDEF89AB =
      _mm512_setr_epi32(4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11);
  __m512i idx_89ABCDEF01234567 =
      _mm512_setr_epi32(8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7);
  __m512i idx_CDEF89AB45670123 =
      _mm512_setr_epi32(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);

  for (iree_uk_int32_t k = 0; k < params->K; ++k) {
    __m512i rhs_i16_0123456789ABCDEF =
        _mm512_cvtepi8_epi16(_mm256_loadu_si256((const __m256i*)rhs_ptr));
    rhs_ptr += 32;
    __m512i lhs_i16_0123456789ABCDEF =
        _mm512_cvtepi8_epi16(_mm256_loadu_si256((const __m256i*)lhs_ptr));
    lhs_ptr += 32;
    __m512i rhs_i16_45670123CDEF89AB = _mm512_permutexvar_epi32(
        idx_45670123CDEF89AB, rhs_i16_0123456789ABCDEF);
    __m512i rhs_i16_89ABCDEF01234567 = _mm512_permutexvar_epi32(
        idx_89ABCDEF01234567, rhs_i16_0123456789ABCDEF);
    __m512i rhs_i16_CDEF89AB45670123 = _mm512_permutexvar_epi32(
        idx_CDEF89AB45670123, rhs_i16_0123456789ABCDEF);
    __m512i lhs_i16_000044448888CCCC =
        _mm512_shuffle_epi32(lhs_i16_0123456789ABCDEF, 0 * 0x55);
    __m512i lhs_i16_111155559999DDDD =
        _mm512_shuffle_epi32(lhs_i16_0123456789ABCDEF, 1 * 0x55);
    __m512i lhs_i16_22226666AAAAEEEE =
        _mm512_shuffle_epi32(lhs_i16_0123456789ABCDEF, 2 * 0x55);
    __m512i lhs_i16_33337777BBBBFFFF =
        _mm512_shuffle_epi32(lhs_i16_0123456789ABCDEF, 3 * 0x55);
    acc_0_0123_4_4567_8_89AB_C_CDEF =
        _mm512_dpwssd_epi32(acc_0_0123_4_4567_8_89AB_C_CDEF,
                            lhs_i16_000044448888CCCC, rhs_i16_0123456789ABCDEF);
    acc_0_4567_4_0123_8_CDEF_C_89AB =
        _mm512_dpwssd_epi32(acc_0_4567_4_0123_8_CDEF_C_89AB,
                            lhs_i16_000044448888CCCC, rhs_i16_45670123CDEF89AB);
    acc_0_89AB_4_CDEF_8_0123_C_4567 =
        _mm512_dpwssd_epi32(acc_0_89AB_4_CDEF_8_0123_C_4567,
                            lhs_i16_000044448888CCCC, rhs_i16_89ABCDEF01234567);
    acc_0_CDEF_4_89AB_8_4567_C_0123 =
        _mm512_dpwssd_epi32(acc_0_CDEF_4_89AB_8_4567_C_0123,
                            lhs_i16_000044448888CCCC, rhs_i16_CDEF89AB45670123);

    acc_1_0123_5_4567_9_89AB_D_CDEF =
        _mm512_dpwssd_epi32(acc_1_0123_5_4567_9_89AB_D_CDEF,
                            lhs_i16_111155559999DDDD, rhs_i16_0123456789ABCDEF);
    acc_1_4567_5_0123_9_CDEF_D_89AB =
        _mm512_dpwssd_epi32(acc_1_4567_5_0123_9_CDEF_D_89AB,
                            lhs_i16_111155559999DDDD, rhs_i16_45670123CDEF89AB);
    acc_1_89AB_5_CDEF_9_0123_D_4567 =
        _mm512_dpwssd_epi32(acc_1_89AB_5_CDEF_9_0123_D_4567,
                            lhs_i16_111155559999DDDD, rhs_i16_89ABCDEF01234567);
    acc_1_CDEF_5_89AB_9_4567_D_0123 =
        _mm512_dpwssd_epi32(acc_1_CDEF_5_89AB_9_4567_D_0123,
                            lhs_i16_111155559999DDDD, rhs_i16_CDEF89AB45670123);

    acc_2_0123_6_4567_A_89AB_E_CDEF =
        _mm512_dpwssd_epi32(acc_2_0123_6_4567_A_89AB_E_CDEF,
                            lhs_i16_22226666AAAAEEEE, rhs_i16_0123456789ABCDEF);
    acc_2_4567_6_0123_A_CDEF_E_89AB =
        _mm512_dpwssd_epi32(acc_2_4567_6_0123_A_CDEF_E_89AB,
                            lhs_i16_22226666AAAAEEEE, rhs_i16_45670123CDEF89AB);
    acc_2_89AB_6_CDEF_A_0123_E_4567 =
        _mm512_dpwssd_epi32(acc_2_89AB_6_CDEF_A_0123_E_4567,
                            lhs_i16_22226666AAAAEEEE, rhs_i16_89ABCDEF01234567);
    acc_2_CDEF_6_89AB_A_4567_E_0123 =
        _mm512_dpwssd_epi32(acc_2_CDEF_6_89AB_A_4567_E_0123,
                            lhs_i16_22226666AAAAEEEE, rhs_i16_CDEF89AB45670123);

    acc_3_0123_7_4567_B_89AB_F_CDEF =
        _mm512_dpwssd_epi32(acc_3_0123_7_4567_B_89AB_F_CDEF,
                            lhs_i16_33337777BBBBFFFF, rhs_i16_0123456789ABCDEF);
    acc_3_4567_7_0123_B_CDEF_F_89AB =
        _mm512_dpwssd_epi32(acc_3_4567_7_0123_B_CDEF_F_89AB,
                            lhs_i16_33337777BBBBFFFF, rhs_i16_45670123CDEF89AB);
    acc_3_89AB_7_CDEF_B_0123_F_4567 =
        _mm512_dpwssd_epi32(acc_3_89AB_7_CDEF_B_0123_F_4567,
                            lhs_i16_33337777BBBBFFFF, rhs_i16_89ABCDEF01234567);
    acc_3_CDEF_7_89AB_B_4567_F_0123 =
        _mm512_dpwssd_epi32(acc_3_CDEF_7_89AB_B_4567_F_0123,
                            lhs_i16_33337777BBBBFFFF, rhs_i16_CDEF89AB45670123);
  }
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 0, 0, 4, 4, 8, 8, 12, 12,
                                           acc_0_0123_4_4567_8_89AB_C_CDEF);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 0, 4, 4, 0, 8, 12, 12, 8,
                                           acc_0_4567_4_0123_8_CDEF_C_89AB);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 0, 8, 4, 12, 8, 0, 12, 4,
                                           acc_0_89AB_4_CDEF_8_0123_C_4567);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 0, 12, 4, 8, 8, 4, 12, 0,
                                           acc_0_CDEF_4_89AB_8_4567_C_0123);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 1, 0, 5, 4, 9, 8, 13, 12,
                                           acc_1_0123_5_4567_9_89AB_D_CDEF);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 1, 4, 5, 0, 9, 12, 13, 8,
                                           acc_1_4567_5_0123_9_CDEF_D_89AB);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 1, 8, 5, 12, 9, 0, 13, 4,
                                           acc_1_89AB_5_CDEF_9_0123_D_4567);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 1, 12, 5, 8, 9, 4, 13, 0,
                                           acc_1_CDEF_5_89AB_9_4567_D_0123);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 2, 0, 6, 4, 10, 8, 14, 12,
                                           acc_2_0123_6_4567_A_89AB_E_CDEF);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 2, 4, 6, 0, 10, 12, 14, 8,
                                           acc_2_4567_6_0123_A_CDEF_E_89AB);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 2, 8, 6, 12, 10, 0, 14, 4,
                                           acc_2_89AB_6_CDEF_A_0123_E_4567);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 2, 12, 6, 8, 10, 4, 14, 0,
                                           acc_2_CDEF_6_89AB_A_4567_E_0123);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 3, 0, 7, 4, 11, 8, 15, 12,
                                           acc_3_0123_7_4567_B_89AB_F_CDEF);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 3, 4, 7, 0, 11, 12, 15, 8,
                                           acc_3_4567_7_0123_B_CDEF_F_89AB);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 3, 8, 7, 12, 11, 0, 15, 4,
                                           acc_3_89AB_7_CDEF_B_0123_F_4567);
  iree_uk_avx512_storeu_4x128_to_16x16xi32(out_ptr, 3, 12, 7, 8, 11, 4, 15, 0,
                                           acc_3_CDEF_7_89AB_B_4567_F_0123);
}
