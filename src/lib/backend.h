#ifndef SC_LIB_BACKEND_H
#define SC_LIB_BACKEND_H

/**
 * @file backend.h
 * @author SWPP TAs (swpp@sf.snu.ac.kr)
 * @brief Assembly emission module
 * @version 2026.1.1
 * @date 2026-04-17
 * @copyright Copyright (c) 2022-2026 SWPP TAs
 */

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetMachine.h"
#include <memory>
#include <string_view>
using namespace llvm;

namespace sc::backend {

class Backend {
private:
  std::unique_ptr<Module> M;
  ModuleAnalysisManager &MAM;
  std::unique_ptr<TargetMachine> TM;
  std::unique_ptr<ToolOutputFile> Out;

  static void initializeBackend();
  void setUpTargetMachine();
  void createOutputStream(StringRef output_filename);
  void runIRPasses();
  void runMachinePasses();

public:
  Backend(std::unique_ptr<Module> &&M, ModuleAnalysisManager &MAM,
          std::string_view _output_filename);
  void run();
};

} // namespace sc::backend

#endif // SC_LIB_BACKEND_H
