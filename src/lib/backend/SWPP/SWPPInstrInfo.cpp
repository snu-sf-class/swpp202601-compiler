#include "SWPPInstrInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

SWPPInstrInfo::SWPPInstrInfo(unsigned CFSetupOpcode, unsigned CFDestroyOpcode,
                             unsigned CatchRetOpcode, unsigned ReturnOpcode)
    : TargetInstrInfo(RI, CFSetupOpcode, CFDestroyOpcode, CatchRetOpcode,
                      ReturnOpcode) {
  InitMCInstrInfo(SWPPDescs.Insts, SWPPInstrNameIndices.data(),
                  SWPPInstrNameData, nullptr, nullptr,
                  SWPP::INSTRUCTION_LIST_END);
}

// Spill Code
// Create new virtual register for calculate pointer of spilled register
// Ptr = const %stack.FrameIndex     : calculate pointer
// store 8 SrcReg(isKill) Ptr(Kill)  : if scalar
// vstore SrcReg(isKill) Ptr(Kill)   : if vector
// Ptr is killed because it will not use after spill code
void SWPPInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI, Register SrcReg,
    bool isKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
    MachineInstr::MIFlag Flags) const {
  Register Ptr =
      MBB.getParent()->getRegInfo().createVirtualRegister(RI.getRegClass(0));
  if (RC->getID() == 0) { // if scalar
    BuildMI(MBB, MBBI, nullptr, get(SWPP::CONST), Ptr)
        .addFrameIndex(FrameIndex);
    BuildMI(MBB, MBBI, nullptr, get(SWPP::STORE))
        .addImm(8)
        .addReg(SrcReg, getKillRegState(isKill))
        .addReg(Ptr, getKillRegState(true));
  } else if (RC->getID() == 1) { // if vector
    BuildMI(MBB, MBBI, nullptr, get(SWPP::CONST), Ptr)
        .addFrameIndex(FrameIndex);
    BuildMI(MBB, MBBI, nullptr, get(SWPP::VSTORE))
        .addReg(SrcReg, getKillRegState(isKill))
        .addReg(Ptr, getKillRegState(true));
  } else
    llvm_unreachable("Register is either gen or vec");
}

// Spill Code
// Create new virtual register for calculate pointer of spilled register
// Ptr = const %stack.FrameIndex : calculate pointer
// DestReg = load 8 Ptr(Kill)    : if scalar
// DestReg = vload Ptr(Kill)     : if vector
// Ptr is killed because it will not use after spill code
void SWPPInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                         MachineBasicBlock::iterator MBBI,
                                         Register DestReg, int FrameIndex,
                                         const TargetRegisterClass *RC,
                                         Register VReg, unsigned SubReg,
                                         MachineInstr::MIFlag Flags) const {
  Register Ptr =
      MBB.getParent()->getRegInfo().createVirtualRegister(RI.getRegClass(0));
  if (RC->getID() == 0) { // if scalar
    BuildMI(MBB, MBBI, nullptr, get(SWPP::CONST), Ptr)
        .addFrameIndex(FrameIndex);
    BuildMI(MBB, MBBI, nullptr, get(SWPP::LOAD), DestReg)
        .addImm(8)
        .addReg(Ptr, getKillRegState(true));
  } else if (RC->getID() == 1) { // if vector
    BuildMI(MBB, MBBI, nullptr, get(SWPP::CONST), Ptr)
        .addFrameIndex(FrameIndex);
    BuildMI(MBB, MBBI, nullptr, get(SWPP::VLOAD), DestReg)
        .addReg(Ptr, getKillRegState(true));
  } else
    llvm_unreachable("Register is either gen or vec");
}

} // namespace llvm
