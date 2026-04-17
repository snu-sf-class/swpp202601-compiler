#ifndef SC_BACKEND_SWPP_TARGETLOWERING_H
#define SC_BACKEND_SWPP_TARGETLOWERING_H

#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class SWPPSubtarget;

class SWPPTargetLowering : public TargetLowering {
public:
  SWPPTargetLowering(const TargetMachine &TM, const SWPPSubtarget &STI);
};

} // namespace llvm

#endif
