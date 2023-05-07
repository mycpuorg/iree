// Copyright 2023 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "iree/hal/utils/libmpi.h"

#include "iree/base/tracing.h"

//===----------------------------------------------------------------------===//
// Dynamic symbol table
//===----------------------------------------------------------------------===//

static iree_status_t iree_hal_mpi_dynamic_symbols_resolve_all(
    iree_dynamic_library_t* library, iree_hal_mpi_dynamic_symbols_t* syms) {
#define MPI_PFN_DECL(mpiSymbolName, ...)                     \
  {                                                          \
    static const char* kName = #mpiSymbolName;               \
    IREE_RETURN_IF_ERROR(iree_dynamic_library_lookup_symbol( \
        library, kName, (void**)&syms->mpiSymbolName));      \
  }
#include "iree/hal/utils/libmpi_dynamic_symbols.h"
#undef MPI_PFN_DECL
  return iree_ok_status();
}

iree_status_t iree_hal_mpi_library_load(
    iree_allocator_t host_allocator, iree_dynamic_library_t** out_library,
    iree_hal_mpi_dynamic_symbols_t* out_syms) {
  IREE_ASSERT_ARGUMENT(out_library);
  IREE_ASSERT_ARGUMENT(out_syms);
  IREE_TRACE_ZONE_BEGIN(z0);
  *out_library = NULL;
  memset(out_syms, 0, sizeof(*out_syms));

  static const char* kMPILoaderSearchNames[] = {
#if defined(IREE_PLATFORM_WINDOWS)
    "msmpi.dll",
#else
    "libmpi.so",
#endif  // IREE_PLATFORM_WINDOWS
  };

  // Try to find and load the library. Will fail if the library is not found (in
  // which case we want to report that specially) or with some other error
  // (symbol conflicts, missing deps, etc).
  iree_dynamic_library_t* library = NULL;
  iree_status_t status = iree_dynamic_library_load_from_files(
      IREE_ARRAYSIZE(kMPILoaderSearchNames), kMPILoaderSearchNames,
      IREE_DYNAMIC_LIBRARY_FLAG_NONE, host_allocator, &library);
  if (iree_status_is_not_found(status)) {
    iree_status_ignore(status);
    status = iree_make_status(
        IREE_STATUS_UNAVAILABLE,
        "MPI runtime library not available; ensure installed and the shared "
        "library is on your PATH/LD_LIBRARY_PATH (msmpi.dll/libmpi.so)");
  }

  // Resolve required symbols. Will fail if any required symbol is missing.
  if (iree_status_is_ok(status)) {
    status = iree_hal_mpi_dynamic_symbols_resolve_all(library, out_syms);
  }

  if (iree_status_is_ok(status)) {
    *out_library = library;
  } else {
    memset(out_syms, 0, sizeof(*out_syms));
    iree_dynamic_library_release(library);
  }
  IREE_TRACE_ZONE_END(z0);
  return status;
}

//===----------------------------------------------------------------------===//
// Error handling utilities
//===----------------------------------------------------------------------===//

// We convert MPI_SUCCESS to IREE_STATUS_SUCCESS, and MPI errors to
// IREE_STATUS_INTERNAL and print the MPI error code.
//
// TODO(cascaval): use MPI_Error to print the actual error or consider
// mapping to other error codes.
iree_status_t iree_hal_mpi_result_to_status(
    iree_hal_mpi_dynamic_symbols_t* syms, int result, const char* file,
    uint32_t line) {
  iree_status_code_t code;
  switch (result) {
    case 0:  // MPI_SUCCESS
      return iree_ok_status();
    default:
      code = IREE_STATUS_INTERNAL;
      break;
  }
  return iree_make_status_with_location(file, line, code, "MPI error %d",
                                        result);
}
