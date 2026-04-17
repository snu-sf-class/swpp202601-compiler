#ifndef SC_BACKEND_GV_ELIMINATE_H
#define SC_BACKEND_GV_ELIMINATE_H

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
using namespace llvm;

namespace sc::backend::gv_elim {

class GVEliminatePass : public PassInfoMixin<GVEliminatePass> {
public:
  PreservedAnalyses run(Module &, ModuleAnalysisManager &);
};

} // namespace sc::backend::gv_elim

#endif // SC_BACKEND_GV_ELIMINATE_H
