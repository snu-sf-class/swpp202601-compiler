#include "gep_eliminate.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include <cstdint>
using namespace llvm;

namespace sc::backend::gep_elim {

PreservedAnalyses GEPEliminatePass::run(Module &M, ModuleAnalysisManager &MAM) {
  IntegerType *Int64Ty = Type::getInt64Ty(M.getContext());
  SmallVector<GetElementPtrInst *, 16> TrashBin;

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *GEPI = dyn_cast<GetElementPtrInst>(&I)) {
          if (!GEPI->getType()->isPointerTy())
            report_fatal_error("parallel GEP calculation not supported");

          SmallVector<Instruction *> Insts;

          Value *PtrOp = GEPI->getPointerOperand();
          Instruction *PtrToInt =
              CastInst::CreateBitOrPointerCast(PtrOp, Int64Ty, "", GEPI);
          Insts.push_back(PtrToInt);

          int64_t Offset = 0;
          for (gep_type_iterator GTI = gep_type_begin(GEPI),
                                 E = gep_type_end(GEPI);
               GTI != E; ++GTI) {
            if (GTI.isStruct() || GTI.isVector())
              report_fatal_error("Struct or Vector GEP not supported");

            Value *Idx = GTI.getOperand();
            auto ElementSize = static_cast<int64_t>(
                GTI.getSequentialElementStride(M.getDataLayout()));

            if (auto *CI = dyn_cast<ConstantInt>(Idx)) {
              Offset += ElementSize * CI->getSExtValue();
              continue;
            }

            if (Offset != 0) {
              ConstantInt *ConstOffset = ConstantInt::get(Int64Ty, Offset);
              Instruction *AddOffset = BinaryOperator::CreateAdd(
                  Insts.back(), ConstOffset, "", GEPI);
              Insts.push_back(AddOffset);
              Offset = 0;
            }

            Value *Base = Insts.back();

            Instruction *OpInt64 =
                CastInst::CreateSExtOrBitCast(Idx, Int64Ty, "", GEPI);
            Insts.push_back(OpInt64);
            if (ElementSize != 1) {
              ConstantInt *ConstSize = ConstantInt::get(Int64Ty, ElementSize);
              Instruction *MulOp =
                  BinaryOperator::CreateMul(OpInt64, ConstSize, "", GEPI);
              Insts.push_back(MulOp);
            }

            Instruction *AddOp =
                BinaryOperator::CreateAdd(Base, Insts.back(), "", GEPI);
            Insts.push_back(AddOp);
          }

          if (Offset != 0) {
            ConstantInt *ConstOffset = ConstantInt::get(Int64Ty, Offset);
            Instruction *Add =
                BinaryOperator::CreateAdd(Insts.back(), ConstOffset, "", GEPI);
            Insts.push_back(Add);
          }

          Instruction *IntToPtr = CastInst::CreateBitOrPointerCast(
              Insts.back(), GEPI->getType(), "", GEPI);
          GEPI->replaceAllUsesWith(IntToPtr);
          TrashBin.push_back(GEPI);
        }
      }
    }
  }

  if (TrashBin.empty())
    return PreservedAnalyses::all();

  for (auto *I : TrashBin)
    I->eraseFromParent();

  return PreservedAnalyses::none();
}

} // namespace sc::backend::gep_elim
