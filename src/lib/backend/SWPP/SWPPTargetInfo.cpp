#include "SWPPTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/TargetParser/Triple.h"

namespace llvm {

Target &getTheSWPPTarget() {
  static Target TheSWPPTarget;
  return TheSWPPTarget;
}

void InitializeSWPPTargetInfo() {
  TargetRegistry::RegisterTarget(
      getTheSWPPTarget(), "swpp", "SWPP", "SWPP",
      [](Triple::ArchType) { return true; }, false);
}

} // namespace llvm
