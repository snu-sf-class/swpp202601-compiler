#ifndef SC_BACKEND_SWPP_FRAMELOWERING_H
#define SC_BACKEND_SWPP_FRAMELOWERING_H

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/Support/Alignment.h"

namespace llvm {

class SWPPSubtarget;

class SWPPFrameLowering : public TargetFrameLowering {
public:
  // Stack growth downwards, No Aligment rules on stack, Local Area offset is 0
  SWPPFrameLowering()
      : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(1), 0) {}
  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  bool hasFPImpl(const MachineFunction &MF) const override;
};

} // namespace llvm

#endif
