#include "mir_printer.h"
#include "SWPP/SWPPInstrInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/ErrorHandling.h"

#include <cstdint>
using namespace llvm;

namespace sc::backend::mirprinter {

char MIRPrinterWrapperPass::ID = 0;

void MIRPrinterImpl::emitOperand(const MachineOperand &MO) {
  switch (MO.getType()) {
  case MachineOperand::MO_Immediate:
    // Emit integer immediate (constant)
    // Todo.. how to handle negative immediate?
    *OS << static_cast<uint64_t>(MO.getImm());
    break;
  case MachineOperand::MO_Predicate:
    // Emit predicate (ex. eq, ult, slt ...)
    *OS << static_cast<CmpInst::Predicate>(MO.getPredicate());
    break;
  case MachineOperand::MO_MachineBasicBlock:
    // Emit BasicBlock's name
    *OS << "." << MO.getMBB()->getName();
    break;
  case MachineOperand::MO_ExternalSymbol:
    // Emit called function name
    *OS << MO.getSymbolName();
    break;
  case MachineOperand::MO_Register: {
    // Emit register name
    Register Reg = MO.getReg();
    if (Reg.isPhysical()) {
      *OS << TRI->getName(Reg.asMCReg());
      break;
    }
    report_fatal_error("unable to emit virtual register");
  }
  default:
    report_fatal_error("Not Supported Machine Operand");
  }
}

void MIRPrinterImpl::emitInstruction(const MachineInstr &MI) {
  // We don't need to emit IMPLICIT_DEF (just definition)
  if (MI.getOpcode() == SWPP::IMPLICIT_DEF)
    return;
  unsigned StartOp = 0;
  unsigned e = MI.getNumOperands();
  // Check def exists ( def = opcode src1 ... )
  while (StartOp < e) {
    const MachineOperand &MO = MI.getOperand(StartOp);
    if (!MO.isReg() || !MO.isDef())
      break;
    if (StartOp != 0)
      *OS << ", ";
    emitOperand(MO);
    ++StartOp;
  }
  if (StartOp != 0)
    *OS << " = ";
  // Print instruction name
  *OS << TII->getName(MI.getOpcode()) << " ";
  for (unsigned i = StartOp; i != e; ++i) {
    const MachineOperand &MO = MI.getOperand(i);
    emitOperand(MO);
    *OS << " ";
  }
  *OS << "\n";
}

void MIRPrinterImpl::emitFunctionBody() {
  // Emit function start, name and number of arguments
  *OS << "start " << MF->getName() << " " << MF->getFunction().arg_size()
      << ":\n";
  for (auto &MBB : *MF) {
    // Emit BasicBlock name
    *OS << "." << MBB.getName() << ":\n";
    for (auto &MI : MBB)
      emitInstruction(MI);
  }
  // Emit function end
  *OS << "end " << MF->getName() << "\n\n";
}

bool MIRPrinterImpl::run(MachineFunction &MF) {
  this->MF = &MF;
  this->MRI = &MF.getRegInfo();
  this->TII = MF.getSubtarget().getInstrInfo();
  this->TRI = MF.getSubtarget().getRegisterInfo();
  const Function &F = MF.getFunction();
  if (F.isDeclaration())
    return false;
  emitFunctionBody();
  return false;
}

} // namespace sc::backend::mirprinter
