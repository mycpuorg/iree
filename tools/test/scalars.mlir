// RUN: (iree-compile --iree-hal-target-backends=vmvx %s | iree-run-module --entry_function=scalar --function_input=42) | FileCheck %s
// RUN: iree-compile --iree-hal-target-backends=vmvx %s | iree-benchmark-module --device=local-task --entry_function=scalar --function_input=42 | FileCheck --check-prefix=BENCHMARK %s
// RUN: (iree-run-mlir --iree-hal-target-backends=vmvx %s --function_input=42) | FileCheck %s

// BENCHMARK-LABEL: BM_scalar
// CHECK-LABEL: EXEC @scalar
func.func @scalar(%arg0 : i32) -> i32 {
  return %arg0 : i32
}
// CHECK: i32=42
