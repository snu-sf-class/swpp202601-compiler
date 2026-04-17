#include "SWPPMCTargetDesc.h"
#include "SWPPInstrInfo.h"
#include "SWPPSubtarget.h"
#include "SWPPTargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/TargetParser/Triple.h"

#define GET_REGINFO_MC_DESC
#include "lib/backend/SWPP/SWPPGenRegisterInfo.inc"

namespace llvm {

static inline MCInstrInfo *createSWPPMCInstrInfo() {
  auto *X = new MCInstrInfo();
  InitSWPPMCInstrInfo(X);
  return X;
}

static inline MCRegisterInfo *createSWPPMCRegisterInfo(const Triple &TT) {
  auto *X = new MCRegisterInfo();
  InitSWPPMCRegisterInfo(X, 0);
  return X;
}

static inline MCSubtargetInfo *
createSWPPMCSubtagetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  return createSWPPMCSubtargetInfoImpl(TT, CPU, CPU, FS);
}

void InitializeSWPPTargetMC() {
  Target *T = &getTheSWPPTarget();
  RegisterMCAsmInfo<SWPPMCAsmInfo> X(*T);
  TargetRegistry::RegisterMCInstrInfo(*T, createSWPPMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(*T, createSWPPMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(*T, createSWPPMCSubtagetInfo);
}

} // namespace llvm
