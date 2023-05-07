// Copyright 2023 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Implements optional canonicalization patterns for StableHLO ops.

#include <cassert>
#include <functional>
#include <numeric>

#include "iree/compiler/InputConversion/StableHLO/Preprocessing/Passes.h"
#include "iree/compiler/InputConversion/StableHLO/Preprocessing/Rewriters.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Matchers.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "stablehlo/dialect/StablehloOps.h"

namespace mlir::iree_compiler::stablehlo {

#define GEN_PASS_DEF_STABLEHLOCANONICALIZE
#include "iree/compiler/InputConversion/StableHLO/Preprocessing/Passes.h.inc"

namespace {

// This is an upper limit on how many elements canonicalization patterns are
// allowed to materialize as new constants.
constexpr int64_t kFoldOpEltLimit = 65536;

struct ConcatenateOpCanon final
    : OpRewritePattern<mlir::stablehlo::ConcatenateOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(mlir::stablehlo::ConcatenateOp op,
                                PatternRewriter &rewriter) const override {
    auto type = dyn_cast<RankedTensorType>(op.getType());
    if (!type || !type.hasStaticShape()) return failure();

    size_t numElems = type.getNumElements();
    if (numElems > kFoldOpEltLimit) return failure();

    // Fold concatenate when all inputs are constants.
    OperandRange inputs = op.getInputs();
    SmallVector<DenseElementsAttr> constants(inputs.size());
    for (auto [input, constant] : llvm::zip_equal(inputs, constants)) {
      if (!matchPattern(input, m_Constant(&constant))) {
        return failure();
      }
    }

    uint64_t axis = op.getDimension();
    ArrayRef<int64_t> shape = type.getShape();
    int64_t topSize = std::accumulate(shape.begin(), shape.begin() + axis,
                                      int64_t{1}, std::multiplies<>{});

    SmallVector<Attribute> newElems;
    newElems.reserve(numElems);

    for (int64_t i = 0; i != topSize; ++i) {
      for (ElementsAttr attr : constants) {
        size_t bottomSize = attr.getNumElements() / topSize;
        auto begin = attr.value_begin<Attribute>() + (i * bottomSize);
        newElems.append(begin, begin + bottomSize);
      }
    }

    assert(newElems.size() == numElems);
    rewriter.replaceOpWithNewOp<mlir::stablehlo::ConstantOp>(
        op, DenseElementsAttr::get(op.getType(), newElems));
    return success();
  }
};

struct ConvertOpCanon final : OpRewritePattern<mlir::stablehlo::ConvertOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(mlir::stablehlo::ConvertOp op,
                                PatternRewriter &rewriter) const override {
    // Check if this convert is a noop.
    if (op.getOperand().getType() != op.getType()) return failure();

    rewriter.replaceOp(op, op.getOperand());
    return success();
  }
};

struct DynamicReshapeOpCanon final
    : OpRewritePattern<mlir::stablehlo::DynamicReshapeOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(mlir::stablehlo::DynamicReshapeOp op,
                                PatternRewriter &rewriter) const override {
    // This is a noop when the output type is already a static shape.
    auto type = dyn_cast<RankedTensorType>(op.getType());
    if (!type || !type.hasStaticShape()) return failure();

    rewriter.replaceOpWithNewOp<mlir::stablehlo::ReshapeOp>(op, type,
                                                            op.getOperand());
    return success();
  }
};

struct RealOpCanon final : OpRewritePattern<mlir::stablehlo::RealOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(mlir::stablehlo::RealOp op,
                                PatternRewriter &rewriter) const override {
    auto complex = op.getOperand().getDefiningOp<mlir::stablehlo::ComplexOp>();
    if (!complex) return failure();

    rewriter.replaceOp(op, complex.getLhs());
    return success();
  }
};

struct ImagOpCanon final : OpRewritePattern<mlir::stablehlo::ImagOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(mlir::stablehlo::ImagOp op,
                                PatternRewriter &rewriter) const override {
    auto complex = op.getOperand().getDefiningOp<mlir::stablehlo::ComplexOp>();
    if (!complex) return failure();

    rewriter.replaceOp(op, complex.getRhs());
    return success();
  }
};

struct GetDimensionSizeOpCanon final
    : OpRewritePattern<mlir::stablehlo::GetDimensionSizeOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(mlir::stablehlo::GetDimensionSizeOp op,
                                PatternRewriter &rewriter) const override {
    // Fold get_dimension_size when the queried dim is statically known.
    auto tensorTy = dyn_cast<RankedTensorType>(op.getOperand().getType());
    if (!tensorTy) return failure();

    int64_t dimSize = tensorTy.getDimSize(op.getDimension());
    if (dimSize < 0) return failure();

    auto elemTy = cast<IntegerType>(op.getType().getElementType());
    IntegerAttr elemVal = rewriter.getIntegerAttr(elemTy, dimSize);
    rewriter.replaceOpWithNewOp<mlir::stablehlo::ConstantOp>(
        op, DenseElementsAttr::get(op.getType(), elemVal));
    return success();
  }
};

struct ReshapeOpCanon final : OpRewritePattern<mlir::stablehlo::ReshapeOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(mlir::stablehlo::ReshapeOp op,
                                PatternRewriter &rewriter) const override {
    // Fold noop reshape.
    if (op.getType() == op.getOperand().getType()) {
      rewriter.replaceOp(op, op.getOperand());
      return success();
    }

    // Fold reshape of a constant.
    ElementsAttr cstAttr;
    if (!matchPattern(op.getOperand(), m_Constant(&cstAttr))) {
      return failure();
    }

    if (auto splat = dyn_cast<SplatElementsAttr>(cstAttr)) {
      rewriter.replaceOpWithNewOp<mlir::stablehlo::ConstantOp>(
          op, SplatElementsAttr::get(op.getType(),
                                     splat.getSplatValue<Attribute>()));
      return success();
    }

    auto elements =
        llvm::to_vector_of<Attribute>(cstAttr.getValues<Attribute>());
    rewriter.replaceOpWithNewOp<mlir::stablehlo::ConstantOp>(
        op, DenseElementsAttr::get(op.getType(), elements));
    return success();
  }
};

struct TransposeOpCanon final : OpRewritePattern<mlir::stablehlo::TransposeOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(mlir::stablehlo::TransposeOp op,
                                PatternRewriter &rewriter) const override {
    // Check if this transpose is a noop and use the operand instead.
    auto dims = op.getPermutation().tryGetValues<APInt>();
    if (failed(dims)) return failure();

    // Check if dims is an iota range.
    for (auto [idx, dim] : llvm::enumerate(*dims)) {
      if (idx != dim.getLimitedValue()) {
        return failure();
      }
    }

    rewriter.replaceOp(op, op.getOperand());
    return success();
  }
};

struct StableHLOCanonicalize final
    : impl::StableHLOCanonicalizeBase<StableHLOCanonicalize> {
  void runOnOperation() override {
    MLIRContext *ctx = &getContext();
    RewritePatternSet patterns(ctx);
    populateCanonicalizationPatterns(ctx, &patterns);
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                            std::move(patterns)))) {
      signalPassFailure();
    }
  }
};

}  // namespace
void populateCanonicalizationPatterns(MLIRContext *context,
                                      RewritePatternSet *patterns,
                                      PatternBenefit benefit) {
  patterns->add<ConcatenateOpCanon, ConvertOpCanon, DynamicReshapeOpCanon,
                RealOpCanon, ImagOpCanon, GetDimensionSizeOpCanon,
                ReshapeOpCanon, TransposeOpCanon>(context, benefit);
}
}  // namespace mlir::iree_compiler::stablehlo
