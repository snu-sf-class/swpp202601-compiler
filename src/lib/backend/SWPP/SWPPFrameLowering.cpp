#include "SWPPFrameLowering.h"
#include "SWPPInstrInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include <cstdint>

namespace llvm {

// decreasing stack pointer
// sp = sub sp Size
void SWPPFrameLowering::emitPrologue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  auto Size = static_cast<int64_t>(MFI.getStackSize());
  // if Size is 0, we don't have to adjust stack pointer
  if (Size == 0)
    return;
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  Register FrameReg = TRI.getFrameRegister(MF);
  BuildMI(MBB, MBB.begin(), nullptr, TII.get(SWPP::ESUB), FrameReg)
      .addUse(FrameReg)
      .addImm(Size)
      .addImm(64);
}

// increasing stack pointer
// sp = add sp Size
void SWPPFrameLowering::emitEpilogue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  auto Size = static_cast<int64_t>(MFI.getStackSize());
  // if Size is 0, we don't have to adjust stack pointer
  if (Size == 0)
    return;
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  Register FrameReg = TRI.getFrameRegister(MF);
  BuildMI(MBB, --MBB.getFirstTerminator(), nullptr, TII.get(SWPP::EADD),
          FrameReg)
      .addUse(FrameReg)
      .addImm(Size)
      .addImm(64);
}

// SWPP::sp is Frame Pointer
bool SWPPFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  return true;
}

} // namespace llvm
