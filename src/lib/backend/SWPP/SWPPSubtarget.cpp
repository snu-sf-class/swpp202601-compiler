#include "SWPPSubtarget.h"
#include "SWPPFrameLowering.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"

namespace llvm {

SWPPSubtarget::SWPPSubtarget(const Triple &TT, const StringRef &CPU,
                             const StringRef &FS, const TargetMachine &TM)
    : TargetSubtargetInfo(TT, CPU, CPU, FS, {}, {}, {}, nullptr, nullptr,
                          nullptr, nullptr, nullptr, nullptr),
      TLInfo(TM, *this) {}

} // namespace llvm
