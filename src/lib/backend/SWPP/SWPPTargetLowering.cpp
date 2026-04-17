#include "SWPPTargetLowering.h"
#include "SWPPSubtarget.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

SWPPTargetLowering::SWPPTargetLowering(const TargetMachine &TM,
                                       const SWPPSubtarget &STI)
    : TargetLowering(TM, STI) {
  // Setting for MachineModuleInfo
  setMinFunctionAlignment(Align(1));
  setPrefFunctionAlignment(Align(1));
}

} // namespace llvm
