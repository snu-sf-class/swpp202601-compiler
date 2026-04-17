#ifndef SC_BACKEND_SWPP_TARGETMACHINE_H
#define SC_BACKEND_SWPP_TARGETMACHINE_H

#include "SWPPSubtarget.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Triple.h"
#include <optional>

namespace llvm {

void InitializeSWPPTarget();

class SWPPTargetMachine : public CodeGenTargetMachineImpl {
  // Actually, Subtarget contains InstrInfo, RegisterInfo, TargetLowering,
  // FrameLowering
  SWPPSubtarget Subtarget;

public:
  SWPPTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                    StringRef FS, const TargetOptions &Options,
                    std::optional<Reloc::Model> RM,
                    std::optional<CodeModel::Model> CM,
                    CodeGenOptLevel OL = CodeGenOptLevel::None,
                    bool JIT = false);

  const SWPPSubtarget *getSubtargetImpl(const Function &) const override {
    return &Subtarget;
  }
};

} // namespace llvm

#endif
