#include "const_expr_eliminate.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Casting.h"
using namespace llvm;

namespace {

Instruction *getInsertionPoint(Instruction &UserInst, Use &OpUse) {
  if (auto *PN = dyn_cast<PHINode>(&UserInst)) {
    BasicBlock *Pred = PN->getIncomingBlock(OpUse.getOperandNo());
    return Pred->getTerminator();
  }
  return &UserInst;
}

Instruction *materializeConstantExpr(ConstantExpr *Expr,
                                     Instruction *InsertBefore) {
  Instruction *NewInst = Expr->getAsInstruction();
  NewInst->insertBefore(InsertBefore);

  for (Use &OpUse : NewInst->operands()) {
    if (auto *NestedExpr = dyn_cast<ConstantExpr>(OpUse.get())) {
      Instruction *NestedInst =
          materializeConstantExpr(NestedExpr, NewInst);
      OpUse.set(NestedInst);
    }
  }

  return NewInst;
}

} // namespace

namespace sc::backend::ce_elim {

PreservedAnalyses ConstExprEliminatePass::run(Module &M,
                                              ModuleAnalysisManager &MAM) {
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &Inst : reverse(BB)) {
        for (auto &Use : Inst.operands()) {
          // ConstantExpr includes 'inlined' operations. For example,
          // store i32 %val, ptr getelementptr inbounds ([16 x i32], ptr @arr,
          // i64 0, i64 2) convert to %0 = getelementptr inbounds [16 x i32],
          // ptr @arr, i64 0, i64 2 store i32 %val, ptr %0
          if (auto *Expr = dyn_cast<ConstantExpr>(Use.get())) {
            Instruction *InsertBefore = getInsertionPoint(Inst, Use);
            Instruction *NewInst =
                materializeConstantExpr(Expr, InsertBefore);
            Use.set(NewInst);
          }
        }
      }
    }
  }
  return PreservedAnalyses::none();
}

} // namespace sc::backend::ce_elim
