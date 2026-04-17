#include "backend.h"
#include "backend/SWPP/SWPPMCTargetDesc.h"
#include "backend/SWPP/SWPPTargetInfo.h"
#include "backend/SWPP/SWPPTargetMachine.h"
#include "backend/const_copy_eliminate.h"
#include "backend/const_expr_eliminate.h"
#include "backend/gep_eliminate.h"
#include "backend/gv_eliminate.h"
#include "backend/mir_printer.h"
#include "backend/mir_translator.h"
#include "backend/sext_eliminate.h"
#include "backend/trunc_adjust.h"
#include "print_ir.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/InitializePasses.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include <memory>
#include <string>
#include <system_error>
#include <utility>
using namespace llvm;

namespace sc::backend {

void Backend::initializeBackend() {
  // Initialize SWPP target and related components.
  InitializeSWPPTarget();
  InitializeSWPPTargetInfo();
  InitializeSWPPTargetMC();
  initializeCodeGen(*PassRegistry::getPassRegistry());

  // Configure code generation options.
  auto &OptMap = cl::getRegisteredOptions();
  // Disable critical edge splitting during PHI elimination
  static_cast<cl::opt<bool> *>(OptMap["disable-phi-elim-edge-splitting"])
      ->setValue(true);
  // Aggressively coalesce fall-through blocks exclusively containing copies.
  static_cast<cl::opt<bool> *>(OptMap["join-splitedges"])->setValue(true);
  // Coalesce global copies in favor of keeping local copies.
  static_cast<cl::opt<cl::boolOrDefault> *>(OptMap["join-globalcopies"])
      ->setValue(cl::BOU_TRUE);
  // Perform exhaustive register search bypassing depth and interference
  // cutoffs.
  static_cast<cl::opt<bool> *>(OptMap["exhaustive-register-search"])
      ->setValue(true);

  if (print_ir::isVerbose())
    EnableStatistics();
}

void Backend::setUpTargetMachine() {
  // Set up the Target with the SWPP triple.
  Triple triple("swpp-unknown-unknown");
  std::string error;
  const Target *TheTarget =
      TargetRegistry::lookupTarget(triple.getTriple(), error);
  if (TheTarget == nullptr)
    report_fatal_error(error.c_str());

  // Create the SWPP TargetMachine.
  TM = std::unique_ptr<TargetMachine>(TheTarget->createTargetMachine(
      triple, "", "", TargetOptions(), Reloc::Static, CodeModel::Small,
      CodeGenOptLevel::None));
  if (TM == nullptr)
    report_fatal_error("Could not allocate target machine");
  M->setTargetTriple(triple);
  M->setDataLayout(TM->createDataLayout());
}

void Backend::createOutputStream(StringRef output_filename) {
  // Create an output file stream for the assembly code.
  std::error_code EC;
  Out = std::make_unique<ToolOutputFile>(output_filename, EC,
                                         sys::fs::OF_TextWithCRLF);
  // Error on open file
  if (!Out)
    report_fatal_error(EC.message().c_str());
}

Backend::Backend(std::unique_ptr<Module> &&M, ModuleAnalysisManager &MAM,
                 std::string_view _output_filename)
    : M(std::move(M)), MAM(MAM) {
  initializeBackend();
  setUpTargetMachine();
  createOutputStream(_output_filename);
}

void Backend::runIRPasses() {
  ModulePassManager MPM;
  MPM.addPass(ce_elim::ConstExprEliminatePass());
  MPM.addPass(gep_elim::GEPEliminatePass());
  MPM.addPass(gv_elim::GVEliminatePass());
  MPM.addPass(trunc_adjust::TruncateAdjustPass());
  MPM.addPass(sext_elim::SignExtendEliminatePass());
  MPM.run(*M, MAM);

  print_ir::printIRIfVerbose(*M, "After Backend IRPasses");
}

void Backend::runMachinePasses() {
  // Use the legacy pass manager to run the code generation passes.
  legacy::PassManager PM;

  // Disable intrinsic library information.
  TargetLibraryInfoImpl TLII(TM->getTargetTriple());
  TLII.disableAllFunctions();
  PM.add(new TargetLibraryInfoWrapperPass(TLII));

  // Manage MachineFunctions with MachineModuleInfo.
  PM.add(new MachineModuleInfoWrapperPass(
      &static_cast<CodeGenTargetMachineImpl &>(*TM)));

  // Translate LLVM IR to SWPP Machine IR.
  PM.add(new mirtranslator::MIRTranslatorWrapperPass());
  if (print_ir::isVerbose())
    PM.add(createMachineFunctionPrinterPass(
        outs(), "----------------------------------------\n"
                "#       After Instruction Selection       \n"
                "# ----------------------------------------\n"));

  // Stack Coloring: Merge disjoint stack slots to reduce stack size.
  // This pass requires that each stack slot is marked by LIFETIME_START and
  // LIFETIME_END. These intrinsics are not implemented, so this pass is
  // disabled
  // PM.add(Pass::createPass(&StackColoringID));

  // Track live variables in the function.
  PM.add(Pass::createPass(&LiveVariablesID));

  // Lower PHI nodes by inserting copy instructions.
  PM.add(Pass::createPass(&PHIEliminationID));
  if (print_ir::isVerbose())
    PM.add(createMachineFunctionPrinterPass(
        outs(), "----------------------------------------\n"
                "#         After SSA Deconstruction        \n"
                "# ----------------------------------------\n"));

  // Calculate live intervals for virtual registers.
  PM.add(Pass::createPass(&LiveIntervalsID));

  // Perform register coalescing to merge virtual registers.
  PM.add(Pass::createPass(&RegisterCoalescerID));
  if (print_ir::isVerbose())
    PM.add(createMachineFunctionPrinterPass(
        outs(), "----------------------------------------\n"
                "#        After Register Coalescing        \n"
                "# ----------------------------------------\n"));

  // Greedy register allocator for optimal register assignment.
  // This pass internally runs several other passes, including:
  // 1. Machine Block Frequency Info: Analyzes block execution frequency.
  // 2. Debug Variable Analysis: Removes all DBG_VALUE instructions.
  // 3. Live Stack Slot Analysis: Analyzes the liveness of stack slots.
  // 4. Virtual Register Map: Implements the Spiller interface.
  // 5. Live Register Matrix: Checks for register interference.
  PM.add(Pass::createPass(&RAGreedyLegacyID));
  // PM.add(createDefaultPBQPRegisterAllocator());

  // Rewrite virtual registers to physical registers.
  PM.add(Pass::createPass(&VirtRegRewriterID));
  if (print_ir::isVerbose())
    PM.add(createMachineFunctionPrinterPass(
        outs(), "----------------------------------------\n"
                "#         After Register Allocation       \n"
                "# ----------------------------------------\n"));

  // Color stack slots to reduce stack usage by merging spill slots.
  PM.add(Pass::createPass(&StackSlotColoringID));

  // Propagate copy instructions to reduce unnecessary moves.
  PM.add(Pass::createPass(&MachineCopyPropagationID));

  // Insert prologue and epilogue code to manage function stack frames.
  PM.add(Pass::createPass(&PrologEpilogCodeInserterID));

  // Eliminate CONST, COPY
  PM.add(new cc_eliminate::ConstCopyEliminateWrapperPass());

  if (print_ir::isVerbose())
    PM.add(createMachineFunctionPrinterPass(
        outs(), "----------------------------------------\n"
                "#       After Backend Machine Passes      \n"
                "# ----------------------------------------\n"));

  // Emit assembly code to the output file.
  PM.add(new mirprinter::MIRPrinterWrapperPass(Out->os()));

  // Deallocate MachineFunction resources after use.
  PM.add(createFreeMachineFunctionPass());

  PM.run(*M);
}

void Backend::run() {
  if (verifyModule(*M, &errs()))
    report_fatal_error("broken module");

  runIRPasses();

  runMachinePasses();

  // Write the translated code to the output file.
  Out->keep();

  if (print_ir::isVerbose())
    PrintStatistics(outs());
}

} // namespace sc::backend
