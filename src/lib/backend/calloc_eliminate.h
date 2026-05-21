#ifndef SC_BACKEND_CALLOC_ELIMINATE_H
#define SC_BACKEND_CALLOC_ELIMINATE_H

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
using namespace llvm;

namespace sc::backend::calloc_elim {

class CallocEliminatePass : public PassInfoMixin<CallocEliminatePass> {
public:
  PreservedAnalyses run(Module &, ModuleAnalysisManager &);
};

} // namespace sc::backend::calloc_elim

#endif // SC_BACKEND_CALLOC_ELIMINATE_H
