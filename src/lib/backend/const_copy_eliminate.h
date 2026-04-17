#ifndef SC_BACKEND_CONST_COPY_ELIMINATE
#define SC_BACKEND_CONST_COPY_ELIMINATE

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
using namespace llvm;

namespace sc::backend::cc_eliminate {

class ConstCopyEliminateImpl {
public:
  bool run(MachineFunction &MF);
};

class ConstCopyEliminate : public PassInfoMixin<ConstCopyEliminate> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM) {
    ConstCopyEliminateImpl Impl;
    return Impl.run(MF) ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

class ConstCopyEliminateWrapperPass : public MachineFunctionPass {
private:
  static char ID;

public:
  ConstCopyEliminateWrapperPass() : MachineFunctionPass(ID) {}
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
  bool runOnMachineFunction(MachineFunction &MF) override {
    ConstCopyEliminateImpl Impl;
    return Impl.run(MF);
  }
};

} // namespace sc::backend::cc_eliminate

#endif
