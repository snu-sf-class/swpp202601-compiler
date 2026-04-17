#include "SWPPRegisterInfo.h"
#include "SWPPFrameLowering.h"
#include "SWPPInstrInfo.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/MC/MCRegister.h"
#include "llvm/Support/ErrorHandling.h"
#include <cstdint>

#define GET_REGINFO_TARGET_DESC
#define GET_REGINFO_ENUM
#include "lib/backend/SWPP/SWPPGenRegisterInfo.inc"

namespace llvm {

// Return Register is not present in SWPPTargetMachine (0 = NoRegister)
SWPPRegisterInfo::SWPPRegisterInfo() : SWPPGenRegisterInfo(0) {
  CSR = new MCPhysReg[1];
  CSR[0] = 0;
}

SWPPRegisterInfo::~SWPPRegisterInfo() { delete[] CSR; }

// SWPP target does not use callee-saved registers.
const MCPhysReg *
SWPPRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR;
}

// Argument registers (from 2 to 17) are considered reserved registers.
BitVector SWPPRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  for (unsigned i = 2; i <= 17; i++)
    Reserved.set(i);
  return Reserved;
}

// Eliminates the frame index from the Machine Instruction.
// replaces a CONST operation with an ADD operation, converting it to a stack
// pointer plus offset calculation. For example,
// $r1 = const %stack.0 (DstReg = const MO_FrameIndex(index))
// transform to
// $r1 = add $sp Offset
bool SWPPRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                           int SPAdj, unsigned FIOperandNum,
                                           RegScavenger *RS) const {
  MachineInstr &MI = *II;
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();

  int Index = MI.getOperand(FIOperandNum).getIndex();
  auto Offset = MFI.getObjectOffset(Index) +
                static_cast<int64_t>(MF.getFrameInfo().getStackSize());

  if (MI.getOpcode() != SWPP::CONST)
    report_fatal_error("Const instruction only use FrameIndex Operand");

  Register FrameReg = getFrameRegister(MF);
  Register DstReg = MI.getOperand(0).getReg();

  BuildMI(MBB, II, nullptr, TII.get(SWPP::EADD))
      .addDef(DstReg, getRenamableRegState(true))
      .addUse(FrameReg)
      .addImm(Offset)
      .addImm(64);

  II->eraseFromParent();
  // original instruction is deleted, return true
  return true;
}

// Frame Register is SWPP::sp (stack pointer)
Register SWPPRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return SWPP::sp;
}

} // namespace llvm
