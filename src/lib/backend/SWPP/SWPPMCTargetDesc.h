#ifndef SC_BACKEND_SWPP_MCTARGETDESC_H
#define SC_BACKEND_SWPP_MCTARGETDESC_H

#include "llvm/MC/MCAsmInfo.h"
#include "llvm/TargetParser/Triple.h"

#define GET_REGINFO_ENUM
#include "lib/backend/SWPP/SWPPGenRegisterInfo.inc"

namespace llvm {

void InitializeSWPPTargetMC();

class SWPPMCAsmInfo : public MCAsmInfo {
public:
  explicit SWPPMCAsmInfo(const Triple &TT, const MCTargetOptions &Options) {}
};

} // namespace llvm

#endif
