#include "trunc_adjust.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
using namespace llvm;

namespace sc::backend::trunc_adjust {

Instruction *getMoveInst(Value *Src) {
  auto *MovConst = ConstantInt::get(Src->getType(), 1);
  return BinaryOperator::CreateMul(Src, MovConst);
}

bool TruncateAdjustPass::isTruncSafeIntrinsic(Instruction *Inst) {
  if (auto *CI = dyn_cast<CallInst>(Inst)) {
    auto Name = CI->getCalledFunction()->getName();
    return safeInstrinsics.find(Name) != safeInstrinsics.end();
  }
  return false;
}

bool TruncateAdjustPass::isTruncSafeInst(Instruction *Inst, Value *Ori) {
  if (isTruncSafeIntrinsic(Inst))
    return true;
  if (auto *IEI = dyn_cast<InsertElementInst>(Inst)) {
    if (IEI->getOperand(2) == Ori)
      // if trunc value is used for third operand, we cannot ignore trunc.
      return false;

    // if trunc value is used for second operand, it is safe.
    return true;
  }
  if (auto *SI = dyn_cast<StoreInst>(Inst)) {
    // safe if trunc value type is not i1.
    return (SI->getValueOperand() == Ori) &&
           (Ori->getType()->getIntegerBitWidth() != 1);
  }

  return isa<BinaryOperator>(Inst) || isa<SExtInst>(Inst) ||
         isa<TruncInst>(Inst);
}

bool TruncateAdjustPass::isTruncIgnorable(TruncInst *TI) {
  for (auto *Use : TI->users()) {
    auto *Inst = cast<Instruction>(Use);
    if (!isTruncSafeInst(Inst, TI))
      return false;
  }
  return true;
}

TruncateAdjustPass::TruncateAdjustPass() {
  safeInstrinsics.insert("incr_i64");
  safeInstrinsics.insert("incr_i32");
  safeInstrinsics.insert("incr_i16");
  safeInstrinsics.insert("incr_i8");
  safeInstrinsics.insert("incr_i1");
  safeInstrinsics.insert("decr_i64");
  safeInstrinsics.insert("decr_i32");
  safeInstrinsics.insert("decr_i16");
  safeInstrinsics.insert("decr_i8");
  safeInstrinsics.insert("decr_i1");
  safeInstrinsics.insert("vbcast_i32x8");
  safeInstrinsics.insert("vbcast_i64x4");
}

PreservedAnalyses TruncateAdjustPass::run(Module &M,
                                          ModuleAnalysisManager &MAM) {
  SmallVector<std::pair<TruncInst *, Instruction *>> InsertPairs;
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *TI = dyn_cast<TruncInst>(&I)) {
          if (isTruncIgnorable(TI))
            continue;

          auto *MovInst = getMoveInst(TI);
          InsertPairs.emplace_back(TI, MovInst);
        }
      }
    }
  }

  if (InsertPairs.empty())
    return PreservedAnalyses::all();

  for (auto &[TI, Mov] : InsertPairs) {
    Mov->insertAfter(TI);
    TI->replaceUsesWithIf(Mov,
                          [Mov](auto &Use) { return Use.getUser() != Mov; });
  }

  return PreservedAnalyses::none();
}

} // namespace sc::backend::trunc_adjust
