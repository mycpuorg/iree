# Import the Tensorflow model "$__SOURCE_MODEL_PATH"
iree_import_tf_model(
  TARGET_NAME "$${_PACKAGE_NAME}_$__TARGET_NAME"
  SOURCE "$__SOURCE_MODEL_PATH"
  OUTPUT_MLIR_FILE "$__OUTPUT_PATH"
)