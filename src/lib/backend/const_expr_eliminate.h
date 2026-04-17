#ifndef SC_BACKEND_CONST_EXPR_ELIMINATE_H
#define SC_BACKEND_CONST_EXPR_ELIMINATE_H

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
using namespace llvm;

namespace sc::backend::ce_elim {

class ConstExprEliminatePass : public PassInfoMixin<ConstExprEliminatePass> {
public:
  PreservedAnalyses run(Module &, ModuleAnalysisManager &);
};

} // namespace sc::backend::ce_elim

#endif // SC_BACKEND_CONST_EXPR_ELIMINATE_H
