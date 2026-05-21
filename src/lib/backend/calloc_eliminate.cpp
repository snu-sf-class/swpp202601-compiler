#include "calloc_eliminate.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Casting.h"
using namespace llvm;

namespace sc::backend::calloc_elim {

PreservedAnalyses CallocEliminatePass::run(Module &M,
                                           ModuleAnalysisManager &MAM) {
  IntegerType *Int64Ty = Type::getInt64Ty(M.getContext());
  PointerType *PtrTy = PointerType::get(M.getContext(), 0);
  SmallVector<CallInst *, 16> Callocs;

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *CI = dyn_cast<CallInst>(&I);
        if (CI == nullptr)
          continue;

        Function *Callee = CI->getCalledFunction();
        if (Callee == nullptr || Callee->getName() != "calloc")
          continue;

        Callocs.push_back(CI);
      }
    }
  }

  if (Callocs.empty())
    return PreservedAnalyses::all();

  FunctionCallee Malloc = M.getOrInsertFunction(
      "malloc", FunctionType::get(PtrTy, {Int64Ty}, false));

  for (auto *CI : Callocs) {
    IRBuilder<> Builder(CI);
    Value *Num = Builder.CreateZExtOrTrunc(CI->getArgOperand(0), Int64Ty);
    Value *Size = Builder.CreateZExtOrTrunc(CI->getArgOperand(1), Int64Ty);
    Value *MallocSize = Builder.CreateMul(Num, Size);
    CallInst *MallocCall = Builder.CreateCall(Malloc, {MallocSize});

    CI->replaceAllUsesWith(MallocCall);
  }

  for (auto *I : Callocs)
    I->eraseFromParent();

  Function *Calloc = M.getFunction("calloc");
  if (Calloc != nullptr && Calloc->isDeclaration() && Calloc->use_empty())
    Calloc->eraseFromParent();

  return PreservedAnalyses::none();
}

} // namespace sc::backend::calloc_elim
