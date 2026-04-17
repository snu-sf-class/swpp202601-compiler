#ifndef SC_BACKEND_SEXT_ELIMINATE_H
#define SC_BACKEND_SEXT_ELIMINATE_H

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
using namespace llvm;

namespace sc::backend::sext_elim {

class SignExtendEliminatePass : public PassInfoMixin<SignExtendEliminatePass> {
public:
  PreservedAnalyses run(Module &, ModuleAnalysisManager &);
};

} // namespace sc::backend::sext_elim

#endif // SC_BACKEND_SEXT_ELIMINATE_H
