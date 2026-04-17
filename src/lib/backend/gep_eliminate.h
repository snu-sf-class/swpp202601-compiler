#ifndef SC_BACKEND_GEP_ELIMINATE_H
#define SC_BACKEND_GEP_ELIMINATE_H

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
using namespace llvm;

namespace sc::backend::gep_elim {

class GEPEliminatePass : public PassInfoMixin<GEPEliminatePass> {
public:
  PreservedAnalyses run(Module &, ModuleAnalysisManager &);
};

} // namespace sc::backend::gep_elim

#endif // SC_BACKEND_GEP_ELIMINATE_H
