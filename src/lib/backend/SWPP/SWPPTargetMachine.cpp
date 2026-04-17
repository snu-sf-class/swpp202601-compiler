#include "SWPPTargetMachine.h"
#include "SWPPSubtarget.h"
#include "SWPPTargetInfo.h"
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Triple.h"
#include <optional>

namespace llvm {

namespace {

// DataLayout for SWPPTargetMachine: 64bit pointer, 256bit vector register
StringRef computeDataLayout(const Triple &TT) {
  return "e-m:e-p:64:64-i64:64-n8:16:32:64-v256:256-S128";
}

} // namespace

SWPPTargetMachine::SWPPTargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     std::optional<Reloc::Model> RM,
                                     std::optional<CodeModel::Model> CM,
                                     CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, computeDataLayout(TT), TT, CPU, FS, Options,
                               RM.value_or(Reloc::Static),
                               CM.value_or(CodeModel::Small), OL),
      Subtarget(TT, CPU, FS, *this) {
  // This method initialize MCRegInfo, MCInstrInfo, MCSubtargetInfo, MCAsmInfo
  initAsmInfo();
}

void InitializeSWPPTarget() {
  RegisterTargetMachine<SWPPTargetMachine> X(getTheSWPPTarget());
}

} // namespace llvm
