#include "const_copy_eliminate.h"
#include "SWPP/SWPPInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
using namespace llvm;

namespace sc::backend::cc_eliminate {

char ConstCopyEliminateWrapperPass::ID = 0;

bool ConstCopyEliminateImpl::run(MachineFunction &MF) {
  const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  SmallVector<MachineInstr *> List;

  for (auto &MBB : MF) {
    for (auto II = MBB.begin(), E = MBB.end(); II != E; ++II) {
      MachineInstr &MI = *II;
      unsigned Opcode = MI.getOpcode();
      if (Opcode != SWPP::CONST && Opcode != SWPP::COPY)
        continue;
      Register Dst = MI.getOperand(0).getReg();

      if (MI.getOpcode() == SWPP::CONST) {
        int64_t Imm = MI.getOperand(1).getImm();
        BuildMI(MBB, II, nullptr, TII->get(SWPP::MUL), Dst)
            .addImm(Imm)
            .addImm(1)
            .addImm(64);
      } else {
        Register Src = MI.getOperand(1).getReg();
        const TargetRegisterClass *Sca = TRI->getRegClass(0);
        const TargetRegisterClass *Vec = TRI->getRegClass(1);

        if (Sca->contains(Src, Dst)) {
          BuildMI(MBB, II, nullptr, TII->get(SWPP::MUL), Dst)
              .addReg(Src)
              .addImm(1)
              .addImm(64);
        } else if (Vec->contains(Src, Dst)) {
          BuildMI(MBB, II, nullptr, TII->get(SWPP::VBCAST), Dst)
              .addImm(1)
              .addImm(64);
          BuildMI(MBB, II, nullptr, TII->get(SWPP::VMUL), Dst)
              .addReg(Dst)
              .addReg(Src)
              .addImm(64);
        } else
          report_fatal_error("unable to eliminate virtual register copy");
      }
      List.push_back(&MI);
    }
  }

  if (List.empty())
    return false;

  for (MachineInstr *II : List)
    II->eraseFromParent();

  return true;
}

} // namespace sc::backend::cc_eliminate
