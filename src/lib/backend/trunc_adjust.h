#ifndef SC_BACKEND_TRUNC_ADJUST_H
#define SC_BACKEND_TRUNC_ADJUST_H

#include "llvm/ADT/StringSet.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
using namespace llvm;

namespace sc::backend::trunc_adjust {

class TruncateAdjustPass : public PassInfoMixin<TruncateAdjustPass> {
private:
  StringSet<> safeInstrinsics;

  bool isTruncSafeIntrinsic(Instruction *Inst);
  bool isTruncSafeInst(Instruction *Inst, Value *Ori);
  bool isTruncIgnorable(TruncInst *TI);

public:
  TruncateAdjustPass();
  PreservedAnalyses run(Module &, ModuleAnalysisManager &);
};

} // namespace sc::backend::trunc_adjust

#endif // SC_BACKEND_TRUNC_ADJUST_H
