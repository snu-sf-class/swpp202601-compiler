#ifndef SC_BACKEND_MIR_PRINTER_H
#define SC_BACKEND_MIR_PRINTER_H

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace sc::backend::mirprinter {

class MIRPrinterImpl {
  friend class MIRPrinter;
  friend class MIRPrinterWrapperPass;

private:
  MachineFunction *MF = nullptr;
  raw_ostream *OS = nullptr;
  const TargetRegisterInfo *TRI = nullptr;
  const TargetInstrInfo *TII = nullptr;
  MachineRegisterInfo *MRI = nullptr;
  void emitFunctionBody();
  void emitInstruction(const MachineInstr &);
  void emitOperand(const MachineOperand &);

public:
  MIRPrinterImpl(raw_ostream &OS) { this->OS = &OS; }
  bool run(MachineFunction &);
};

class MIRPrinter : public PassInfoMixin<MIRPrinter> {
private:
  raw_ostream *OS = nullptr;

public:
  MIRPrinter(raw_ostream &OS) { this->OS = &OS; }
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM) {
    MIRPrinterImpl Impl(*OS);
    Impl.run(MF);
    return PreservedAnalyses::all();
  }
};

class MIRPrinterWrapperPass : public MachineFunctionPass {
private:
  static char ID;
  raw_ostream *OS = nullptr;

public:
  MIRPrinterWrapperPass(raw_ostream &OS) : MachineFunctionPass(ID) {
    this->OS = &OS;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    MIRPrinterImpl Impl(*OS);
    Impl.run(MF);
    return false;
  }
};

} // namespace sc::backend::mirprinter

#endif
