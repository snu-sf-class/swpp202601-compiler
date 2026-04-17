#ifndef SC_BACKEND_SWPP_REGISTERINFO_H
#define SC_BACKEND_SWPP_REGISTERINFO_H
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/MC/MCRegister.h"

#define GET_REGINFO_HEADER
#include "lib/backend/SWPP/SWPPGenRegisterInfo.inc"

namespace llvm {

struct SWPPRegisterInfo : public SWPPGenRegisterInfo {
private:
  MCPhysReg *CSR;

public:
  SWPPRegisterInfo();
  ~SWPPRegisterInfo();

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;
  BitVector getReservedRegs(const MachineFunction &MF) const override;
  bool eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;
  Register getFrameRegister(const MachineFunction &MF) const override;
};

} // namespace llvm

#endif
