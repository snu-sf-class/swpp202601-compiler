#ifndef SC_BACKEND_SWPP_SUBTARGET_H
#define SC_BACKEND_SWPP_SUBTARGET_H

#include "SWPPFrameLowering.h"
#include "SWPPInstrInfo.h"
#include "SWPPRegisterInfo.h"
#include "SWPPTargetLowering.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCInstrItineraries.h"
#include "llvm/MC/MCSchedule.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"

namespace llvm {

struct SWPPMCSubtargetInfo : public MCSubtargetInfo {
  SWPPMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef TuneCPU,
                      StringRef FS, ArrayRef<StringRef> PN,
                      ArrayRef<SubtargetFeatureKV> PF,
                      ArrayRef<SubtargetSubTypeKV> PD,
                      const MCWriteProcResEntry *WPR,
                      const MCWriteLatencyEntry *WL,
                      const MCReadAdvanceEntry *RA, const InstrStage *IS,
                      const unsigned *OC, const unsigned *FP)
      : MCSubtargetInfo(TT, CPU, TuneCPU, FS, PN, PF, PD, WPR, WL, RA, IS, OC,
                        FP) {}
};

static inline MCSubtargetInfo *createSWPPMCSubtargetInfoImpl(const Triple &TT,
                                                             StringRef CPU,
                                                             StringRef TuneCPU,
                                                             StringRef FS) {
  return new SWPPMCSubtargetInfo(TT, CPU, TuneCPU, FS, {}, {}, {}, nullptr,
                                 nullptr, nullptr, nullptr, nullptr, nullptr);
}

class SWPPSubtarget : public TargetSubtargetInfo {
  // InstrInfo contains RegisterInfo
  SWPPInstrInfo InstrInfo;
  SWPPFrameLowering FrameLowering;
  SWPPTargetLowering TLInfo;

public:
  SWPPSubtarget(const Triple &TT, const StringRef &CPU, const StringRef &FS,
                const TargetMachine &TM);

  const SWPPInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const SWPPFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const SWPPTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const SWPPRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
};

} // namespace llvm

#endif
