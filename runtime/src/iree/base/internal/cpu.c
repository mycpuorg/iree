// Copyright 2022 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// NOTE: must be first before _any_ system includes.
#define _GNU_SOURCE

#include "iree/base/internal/cpu.h"

#include "iree/base/target_platform.h"
#include "iree/base/tracing.h"

//===----------------------------------------------------------------------===//
// Platform-specific processor data queries
//===----------------------------------------------------------------------===//

#if defined(IREE_PLATFORM_ANDROID) || defined(IREE_PLATFORM_LINUX)

// NOTE: not all kernel versions have all of the cap bits we need defined so as
// a practice we always define the feature bits we need locally.
#include <sys/auxv.h>

// OR's |field_bit| into |field_value| if |hwcap_bit| is set in |hwcap_value|.
#define IREE_SET_IF_HWCAP(hwcap_value, hwcap_bit, field_value, field_bit) \
  if (iree_all_bits_set(hwcap_value, hwcap_bit)) (field_value) |= (field_bit)

#if defined(IREE_ARCH_ARM_64)

// TODO: support the code in iree/tooling/cpu_features.c that checks to see if
// HWCAP_CPUID is available and directly queries the architectural registers.
// We'll still need the fallback to HWCAP checks but it would allow us to
// detect features even on kernel versions that don't yet understand them.

// https://docs.kernel.org/arm64/elf_hwcaps.html
#define IREE_HWCAP_ASIMDDP (1 << 20)
#define IREE_HWCAP2_I8MM (1 << 13)
static void iree_cpu_query_data_arch_hwcaps(uint32_t hwcap, uint32_t hwcap2,
                                            uint64_t* out_fields) {
  IREE_SET_IF_HWCAP(hwcap, IREE_HWCAP_ASIMDDP, out_fields[0],
                    IREE_CPU_DATA_FIELD_0_AARCH64_HAVE_DOTPROD);
  IREE_SET_IF_HWCAP(hwcap2, IREE_HWCAP2_I8MM, out_fields[0],
                    IREE_CPU_DATA_FIELD_0_AARCH64_HAVE_I8MM);
}

#else
static void iree_cpu_query_data_arch_hwcaps(uint32_t hwcap, uint32_t hwcap2,
                                            uint64_t* out_fields) {
  // Not supported on this arch.
}
#endif  // IREE_ARCH_*

#undef IREE_SET_IF_HWCAP

static void iree_cpu_initialize_from_platform(iree_allocator_t temp_allocator,
                                              uint64_t* out_fields) {
  uint32_t hwcap = getauxval(AT_HWCAP);
  uint32_t hwcap2 = getauxval(AT_HWCAP2);
  iree_cpu_query_data_arch_hwcaps(hwcap, hwcap2, out_fields);
}

#elif defined(IREE_PLATFORM_MACOS) || defined(IREE_PLATFORM_IOS)

#include <sys/sysctl.h>
#include <sys/types.h>

#define IREE_QUERY_SYSCTL(key, field_value, field_bit)            \
  do {                                                            \
    int64_t result = 0;                                           \
    size_t result_size = sizeof result;                           \
    if (0 == sysctlbyname(key, &result, &result_size, NULL, 0)) { \
      if (result) field_value |= field_bit;                       \
    }                                                             \
  } while (0)

static void iree_cpu_initialize_from_platform(iree_allocator_t temp_allocator,
                                              uint64_t* out_fields) {
#if defined(IREE_ARCH_ARM_64)
  IREE_QUERY_SYSCTL("hw.optional.arm.FEAT_DotProd", out_fields[0],
                    IREE_CPU_DATA_FIELD_0_AARCH64_HAVE_DOTPROD);
  IREE_QUERY_SYSCTL("hw.optional.arm.FEAT_I8MM", out_fields[0],
                    IREE_CPU_DATA_FIELD_0_AARCH64_HAVE_I8MM);
#endif
}

#else

static void iree_cpu_initialize_from_platform(iree_allocator_t temp_allocator,
                                              uint64_t* out_fields) {
  // No implementation available. CPU data will be all zeros.
}

#endif  // IREE_PLATFORM_*

//===----------------------------------------------------------------------===//
// Architecture-specific string lookup
//===----------------------------------------------------------------------===//

#define IREE_TEST_FIELD_BIT(field_key, field_value, bit_value)          \
  if (iree_string_view_equal(key, IREE_SV(field_key))) {                \
    *out_value = iree_all_bits_set((field_value), (bit_value)) ? 1 : 0; \
    return true;                                                        \
  }

#if defined(IREE_ARCH_ARM_64)

static bool iree_cpu_lookup_data_by_key_for_arch(
    const uint64_t* fields, iree_string_view_t key,
    int64_t* IREE_RESTRICT out_value) {
  IREE_TEST_FIELD_BIT("dotprod", fields[0],
                      IREE_CPU_DATA_FIELD_0_AARCH64_HAVE_DOTPROD);
  IREE_TEST_FIELD_BIT("i8mm", fields[0],
                      IREE_CPU_DATA_FIELD_0_AARCH64_HAVE_I8MM);
  return false;
}

#else

static bool iree_cpu_lookup_data_by_key_for_arch(
    const uint64_t* fields, iree_string_view_t key,
    int64_t* IREE_RESTRICT out_value) {
  // Not yet implemented for this architecture.
  return false;
}

#endif  // IREE_ARCH_*

#undef IREE_TEST_FIELD_BIT

//===----------------------------------------------------------------------===//
// Processor data query
//===----------------------------------------------------------------------===//

static iree_alignas(64) uint64_t
    iree_cpu_data_cache_[IREE_CPU_DATA_FIELD_COUNT] = {0};

void iree_cpu_initialize(iree_allocator_t temp_allocator) {
  IREE_TRACE_ZONE_BEGIN(z0);
  memset(iree_cpu_data_cache_, 0, sizeof(iree_cpu_data_cache_));
  iree_cpu_initialize_from_platform(temp_allocator, iree_cpu_data_cache_);
  IREE_TRACE_ZONE_END(z0);
}

void iree_cpu_initialize_with_data(iree_host_size_t field_count,
                                   const uint64_t* fields) {
  memset(iree_cpu_data_cache_, 0, sizeof(iree_cpu_data_cache_));
  memcpy(iree_cpu_data_cache_, fields,
         iree_min(field_count, IREE_ARRAYSIZE(iree_cpu_data_cache_)) *
             sizeof(*iree_cpu_data_cache_));
}

const uint64_t* iree_cpu_data_fields(void) { return iree_cpu_data_cache_; }

uint64_t iree_cpu_data_field(iree_host_size_t field) {
  if (IREE_UNLIKELY(field >= IREE_ARRAYSIZE(iree_cpu_data_cache_))) return 0;
  return iree_cpu_data_cache_[field];
}

void iree_cpu_read_data(iree_host_size_t field_count, uint64_t* out_fields) {
  memset(out_fields, 0, field_count * sizeof(*out_fields));
  memcpy(out_fields, iree_cpu_data_cache_,
         iree_min(field_count, IREE_ARRAYSIZE(iree_cpu_data_cache_)) *
             sizeof(*out_fields));
}

iree_status_t iree_cpu_lookup_data_by_key(iree_string_view_t key,
                                          int64_t* IREE_RESTRICT out_value) {
  if (!iree_cpu_lookup_data_by_key_for_arch(iree_cpu_data_cache_, key,
                                            out_value)) {
    return iree_make_status(IREE_STATUS_NOT_FOUND,
                            "CPU data key '%.*s' not found", (int)key.size,
                            key.data);
  }
  return iree_ok_status();
}

//===----------------------------------------------------------------------===//
// Processor identification
//===----------------------------------------------------------------------===//

#if defined(IREE_PLATFORM_ANDROID) || defined(IREE_PLATFORM_LINUX)

#include <sched.h>

iree_cpu_processor_id_t iree_cpu_query_processor_id(void) {
  // This path is relatively portable and should work on linux/bsd/etc-likes.
  // We may want to use getcpu when available so that we can get the group ID.
  // https://man7.org/linux/man-pages/man3/sched_getcpu.3.html
  //
  // libc implementations can use vDSO and other fun stuff to make this really
  // cheap: http://git.musl-libc.org/cgit/musl/tree/src/sched/sched_getcpu.c
  int id = sched_getcpu();
  return id != -1 ? id : 0;
}

#elif defined(IREE_PLATFORM_WINDOWS)

iree_cpu_processor_id_t iree_cpu_query_processor_id(void) {
  PROCESSOR_NUMBER pn;
  GetCurrentProcessorNumberEx(&pn);
  return 64 * pn.Group + pn.Number;
}

#else

// No implementation.
// We could allow an iree/base/config.h override to externalize this.
iree_cpu_processor_id_t iree_cpu_query_processor_id(void) { return 0; }

#endif  // IREE_PLATFORM_*

void iree_cpu_requery_processor_id(iree_cpu_processor_tag_t* IREE_RESTRICT tag,
                                   iree_cpu_processor_id_t* IREE_RESTRICT
                                       processor_id) {
  IREE_ASSERT_ARGUMENT(tag);
  IREE_ASSERT_ARGUMENT(processor_id);

  // TODO(benvanik): set a frequency for this and use a coarse timer
  // (CLOCK_MONOTONIC_COARSE) to do a ~4-10Hz refresh. We can store the last
  // query time and the last processor ID in the tag and only perform the query
  // if it has changed.

  *processor_id = iree_cpu_query_processor_id();
}
