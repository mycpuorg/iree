// RUN: iree-dialects-opt --transform-dialect-interpreter %s | FileCheck %s

// CHECK-LABEL: func @matmul_tensors
// CHECK-NOT: linalg
// CHECK: llvm
func.func @matmul_tensors(
  %arg0: tensor<128x128xf32>, %arg1: tensor<128x128xf32>, %arg2: tensor<128x128xf32> { linalg.inplaceable = true})
    -> tensor<128x128xf32> {
  %0 = linalg.matmul  ins(%arg0, %arg1: tensor<128x128xf32>, tensor<128x128xf32>)
                     outs(%arg2: tensor<128x128xf32>)
    -> tensor<128x128xf32>

  return %0 : tensor<128x128xf32>
}


transform.sequence failures(propagate) {
^bb1(%module_op: !transform.any_op):
  %0 = transform.structured.match ops{["linalg.matmul"]} in %module_op : (!transform.any_op) -> !transform.any_op
  %1, %loops:3 = transform.structured.tile %0 [4, 4, 4]
      : (!transform.any_op) -> (!transform.any_op, !transform.any_op, !transform.any_op, !transform.any_op)
  %2 = get_parent_op %1 {isolated_from_above} : (!transform.any_op) -> !transform.any_op
  transform.structured.vectorize %2 { vectorize_padding } : (!transform.any_op) -> !transform.any_op
  %module_op1 = transform.bufferization.one_shot_bufferize layout{IdentityLayoutMap} %module_op
    {bufferize_function_boundaries = true} : (!transform.any_op) -> !transform.any_op

  %func = transform.structured.match ops{["func.func"]} in %module_op1
    : (!transform.any_op) -> !transform.any_op
  transform.apply_patterns to %func {
      transform.apply_patterns.vector.lower_contraction lowering_strategy = "outerproduct"
  } : !transform.any_op
  transform.apply_patterns to %func {
      transform.apply_patterns.vector.lower_transpose lowering_strategy = "shuffle_1d"
  } : !transform.any_op

  lower_to_llvm %module_op1 : (!transform.any_op) -> !transform.any_op
}
