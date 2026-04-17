#include "lib.h"

#include "lib/backend.h"
#include "lib/opt.h"
#include "lib/print_ir.h"

#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Target/TargetMachine.h"

#include <memory>

using namespace static_error;

namespace sc {
template <typename E>
SWPPCompilerError::SWPPCompilerError(Error<E> &&__err) noexcept {
  using namespace std::string_literals;
  message = "swpp-compiler crashed: "s.append(__err.what());
}

std::expected<bool, SWPPCompilerError>
compile(const std::string_view __input_filename,
        const std::string_view __output_filename,
        const bool __verbose_printing = false) noexcept {

  llvm::LLVMContext Context;

  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;

  llvm::PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  if (__verbose_printing) {
    print_ir::setVerbose();
  }

  llvm::SMDiagnostic Err;
  std::unique_ptr<llvm::Module> parse_result =
      llvm::parseIRFile(__input_filename, Err, Context);
  if (!parse_result) {
    Err.print("swpp-compiler", llvm::errs());
    return false;
  }

  auto optimize_result = sc::opt::optimizeIR(std::move(parse_result), MAM)
                             .transform_error([](auto &&err) {
                               return SWPPCompilerError(std::move(err));
                             });

  sc::backend::Backend(std::move(optimize_result.value()), MAM,
                       __output_filename)
      .run();

  return true;
}
} // namespace sc
