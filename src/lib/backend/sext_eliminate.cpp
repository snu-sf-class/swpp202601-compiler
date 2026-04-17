#include "sext_eliminate.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <utility>

using namespace llvm;

namespace sc::backend::sext_elim {

PreservedAnalyses SignExtendEliminatePass::run(Module &M,
                                               ModuleAnalysisManager &MAM) {
  SmallVector<std::pair<SExtInst *, BinaryOperator *>> ReplacePairs;

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *SI = dyn_cast<SExtInst>(&I)) {
          Value *Src = SI->getOperand(0);
          if (!Src->getType()->isIntegerTy() || !SI->getType()->isIntegerTy())
            report_fatal_error("SExt only operates on Scalar Integer");
          unsigned BeforeBits = Src->getType()->getIntegerBitWidth();
          unsigned AfterBits = SI->getType()->getIntegerBitWidth();

          auto *ZExt =
              ZExtInst::CreateZExtOrBitCast(Src, SI->getType(), "", &I);
          auto *ShAmount =
              ConstantInt::get(SI->getType(), AfterBits - BeforeBits);
          auto *Shl = BinaryOperator::CreateShl(ZExt, ShAmount, "", &I);
          auto *AShr = BinaryOperator::CreateAShr(Shl, ShAmount);
          ReplacePairs.emplace_back(SI, AShr);
        }
      }
    }
  }

  if (ReplacePairs.empty())
    return PreservedAnalyses::all();

  for (auto &[SI, To] : ReplacePairs)
    ReplaceInstWithInst(SI, To);

  return PreservedAnalyses::none();
}

} // namespace sc::backend::sext_elim
