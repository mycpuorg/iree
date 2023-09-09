// Copyright 2023 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "iree/compiler/Codegen/LLVMCPU/PassDetail.h"
#include "iree/compiler/Codegen/LLVMCPU/Passes.h"
#include "llvm/Support/CommandLine.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Linalg/Transforms/Transforms.h"
#include "mlir/Dialect/Linalg/Utils/Utils.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/MemRef/Transforms/Transforms.h"
#include "mlir/Dialect/SCF/Transforms/Patterns.h"
#include "mlir/Dialect/SCF/Transforms/TileUsingInterface.h"
#include "mlir/Dialect/SCF/Transforms/Transforms.h"
#include "mlir/IR/Iterators.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

#define DEBUG_TYPE "iree-llvmcpu-tile"

namespace mlir {
namespace iree_compiler {
namespace {

/// Builds a proper tile sizes vector for the op.
/// scf::tileUsingSCFForOp expects the num of tile sizes = num of loops. This
/// method returns a proper tile sizes vector for each op during tiling.
static SmallVector<Value> buildTileSizesForOp(OpBuilder &b, Operation *op,
                                              ArrayRef<int64_t> tileSizes) {
  auto tilingOp = cast<TilingInterface>(op);

  SmallVector<int64_t> newTileSizes(tileSizes);
  newTileSizes.resize(tilingOp.getLoopIteratorTypes().size(), /*default=*/0);

  OpBuilder::InsertionGuard guard(b);
  return llvm::map_to_vector(newTileSizes, [&](int64_t size) -> Value {
    return b.create<arith::ConstantIndexOp>(tilingOp->getLoc(), size);
  });
}

/// This pass tiles all the TilingInterface operations. The `tilingLevel` must
/// be specified. It picks the `tilingLevel`-th list as tiling sizes from
/// lowering_config.
struct LLVMCPUTilePass : LLVMCPUTileBase<LLVMCPUTilePass> {
  LLVMCPUTilePass(int64_t tilingLevel = -1) {
    this->tilingLevel.setValue(tilingLevel);
  }
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<arith::ArithDialect, affine::AffineDialect,
                    linalg::LinalgDialect, scf::SCFDialect>();
  }

  void runOnOperation() override;
};

void LLVMCPUTilePass::runOnOperation() {
  if (tilingLevel == -1) {
    LLVM_DEBUG(llvm::dbgs() << "tilingLevel not set, skip tiling\n");
    return;
  }
  MLIRContext *context = &getContext();
  auto funcOp = getOperation();

  SmallVector<Operation *> computeOps = getComputeOps(funcOp);
  FailureOr<IREE::Codegen::LoweringConfigAttr> rootLoweringConfig =
      getLoweringConfig(computeOps);
  if (failed(rootLoweringConfig)) {
    LLVM_DEBUG(llvm::dbgs() << "can't find lowering_config, skip tiling\n");
    return;
  }

  for (auto computeOp : computeOps) {
    auto op = cast<TilingInterface>(computeOp);
    if (op.getLoopIteratorTypes().empty())
      continue;

    // For now do not tile `tensor.pad` operations. The `tensor.pad`
    // operations might be those introduced by the padding-based
    // codegeneration strategy. Those are not meant to be tiled again.
    // Need a better way for handling this, but this works for now.
    if (isa<tensor::PadOp>(computeOp))
      continue;

    LLVM_DEBUG(llvm::dbgs() << "candidate: " << op << "\n");
    SmallVector<int64_t> tileSizes;
    if (auto loweringConfig = getLoweringConfig(op)) {
      tileSizes = loweringConfig.getTileSizeVals(tilingLevel);
    } else {
      tileSizes = rootLoweringConfig.value().getTileSizeVals(tilingLevel);
    }

    if (llvm::all_of(tileSizes, [](int64_t v) { return v == 0; })) {
      LLVM_DEBUG(llvm::dbgs() << "tiling sizes are all zeros, skip tiling\n");
      continue;
    }

    IRRewriter rewriter(context);
    auto options = scf::SCFTilingOptions().setTileSizeComputationFunction(
        [tileSizes](OpBuilder &b, Operation *op) {
          return buildTileSizesForOp(b, op, tileSizes);
        });
    FailureOr<scf::SCFTilingResult> tiledResults =
        scf::tileUsingSCFForOp(rewriter, op, options);
    if (failed(tiledResults))
      continue;
    rewriter.replaceOp(op, tiledResults->replacements);
  }

  RewritePatternSet patterns =
      linalg::getLinalgTilingCanonicalizationPatterns(context);
  scf::populateSCFForLoopCanonicalizationPatterns(patterns);
  tensor::populateFoldTensorEmptyPatterns(patterns);
  memref::populateResolveRankedShapedTypeResultDimsPatterns(patterns);
  context->getLoadedDialect<tensor::TensorDialect>()
      ->getCanonicalizationPatterns(patterns);
  if (failed(applyPatternsAndFoldGreedily(funcOp, std::move(patterns)))) {
    LLVM_DEBUG(llvm::dbgs() << "----- cleanup failed -----\n");
    return signalPassFailure();
  }
}
} // namespace

std::unique_ptr<OperationPass<func::FuncOp>>
createLLVMCPUTilePass(int64_t tilingLevel) {
  return std::make_unique<LLVMCPUTilePass>(tilingLevel);
}

} // namespace iree_compiler
} // namespace mlir
