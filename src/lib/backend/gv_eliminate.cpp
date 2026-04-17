#include "gv_eliminate.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TypeSize.h"
#include <cstdint>
using namespace llvm;

namespace {

constexpr uint64_t START_ADDRESS = 204800UL;

} // namespace

namespace sc::backend::gv_elim {

PreservedAnalyses GVEliminatePass::run(Module &M, ModuleAnalysisManager &MAM) {
  IntegerType *Int64Ty = Type::getInt64Ty(M.getContext());
  uint64_t Acc = 0U;
  SmallVector<GlobalVariable *, 8> GVList;

  for (auto &GV : M.globals()) {
    GVList.push_back(&GV);
    uint64_t Addr = START_ADDRESS + Acc;
    SmallVector<PtrToIntInst *, 2> TrashBin;
    SmallVector<Use *> Uses;
    for (auto &Use : GV.uses())
      Uses.push_back(&Use);

    for (auto *Use : Uses) {
      User *User = Use->getUser();
      if (auto *I = dyn_cast<Instruction>(User)) {
        if (auto *PTII = dyn_cast<PtrToIntInst>(I)) {
          Constant *C = ConstantInt::get(PTII->getType(), Addr, true);
          PTII->replaceAllUsesWith(C);
          TrashBin.push_back(PTII);
        } else {
          ConstantInt *C = ConstantInt::get(Int64Ty, Addr, true);
          CastInst *CI =
              CastInst::CreateBitOrPointerCast(C, GV.getType(), "", I);
          Use->set(CI);
        }
      }
    }

    for (auto *PTII : TrashBin)
      PTII->eraseFromParent();

    TypeSize GVTypeSize = M.getDataLayout().getTypeStoreSize(GV.getValueType());
    if (GVTypeSize.isScalable())
      report_fatal_error("scalable global variable not supported");
    uint64_t GVSize = GVTypeSize.getFixedValue();
    uint64_t AllocSize = (GVSize + 7UL) & (UINT64_MAX - 7UL);
    Acc += AllocSize;
  }

  if (GVList.empty())
    return PreservedAnalyses::all();

  for (GlobalVariable *GV : GVList)
    GV->eraseFromParent();

  if (Acc != 0U) {
    Function *F = M.getFunction("main");
    if (F == nullptr)
      report_fatal_error("no main function to bind the GV");

    FunctionCallee FC = M.getOrInsertFunction(
        "malloc", PointerType::get(M.getContext(), 0), Int64Ty);
    ConstantInt *Const = ConstantInt::get(Int64Ty, Acc, true);
    Instruction *I = F->getEntryBlock().getFirstNonPHI();
    if (I == nullptr)
      report_fatal_error("unable to bind the malloc into the entry block");

    CallInst::Create(FC, {Const}, "", I);
  }

  return PreservedAnalyses::none();
}

} // namespace sc::backend::gv_elim
