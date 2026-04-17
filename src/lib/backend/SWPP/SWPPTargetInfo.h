#ifndef SC_BACKEND_SWPP_TARGETINFO_H
#define SC_BACKEND_SWPP_TARGETINFO_H

#include "llvm/MC/TargetRegistry.h"

namespace llvm {

Target &getTheSWPPTarget();

void InitializeSWPPTargetInfo();

} // namespace llvm

#endif
