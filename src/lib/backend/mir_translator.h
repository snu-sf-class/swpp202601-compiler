#ifndef SC_BACKEND_MIR_TRANSLATOR_H
#define SC_BACKEND_MIR_TRANSLATOR_H

#include "SWPP/SWPPInstrInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachinePassManager.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

using namespace llvm;

namespace sc::backend::mirtranslator {

class MIRTranslatorImpl {
  friend class MIRTranslatorWrapperPass;
  friend class MIRTranslator;

private:
  MachineFunction *MF = nullptr;
  Function *F = nullptr;
  MachineRegisterInfo *MRI = nullptr;
  const TargetRegisterInfo *TRI = nullptr;
  const TargetInstrInfo *TII = nullptr;
  const DataLayout *DL = nullptr;

  // Data structure for Value - Virtual Register mapping
  class ValueToVRegInfo {
  public:
    ValueToVRegInfo() = default;

  private:
    DenseMap<const Value *, Register> ValToVReg;
    Register &insertVRegs(const Value &V) { return ValToVReg[&V]; }

  public:
    Register &getVReg(const Value &V) {
      if (contains(V))
        return ValToVReg.find(&V)->second;
      return insertVRegs(V);
    }
    Register &findVReg(const Value &V) { return ValToVReg.find(&V)->second; }
    bool contains(const Value &V) const { return ValToVReg.contains(&V); }
    void reset() { ValToVReg.clear(); }
  };

  // Value - Virtual Register Map
  ValueToVRegInfo VMap;

  // BasicBlock - MachineBasicBlock Map
  DenseMap<const BasicBlock *, MachineBasicBlock *> BBToMBB;

  // PHI nodes in function
  SmallVector<std::pair<const PHINode *, MachineInstr *>, 4> PendingPHIs;

  // Undef Registers
  SmallVector<Register, 4> UndefRegs;

  using PreCallHandler = std::function<void(const User &)>;
  const std::unordered_map<std::string, PreCallHandler> PreCallHandlers;

  // check vector and is supported
  // return true if vector, false is scalar, abort when unsupported type
  // provided.
  static bool isVector(const Value &V);

  // Algorithm
  // 1. Find virtual register for each operand, create if not exists
  // 2. create MachineInstr
  void translate(const Instruction &Inst);

  // Control Flow
  void translateRet(const User &U);
  void translateBr(const User &U);
  void translateSwitch(const User &U);
  void translatePreCall(const User &U);
  void translateCall(const User &U);
  void translateRCall(const User &U);
  void translatePHI(const User &U);
  void finishPendingPhis();

  // Memory Allocation/Deallocation
  void translateMalloc(const User &U);
  void translateAlloca(const User &U);
  void translateFree(const User &U);

  // Memory Access
  void translateLoad(const User &U);
  void translateALoad(const User &U);
  void translateStore(const User &U);

  // Arithmetic
  void translateUnaryOp(unsigned Opcode, const User &U);
  void translateBinaryOp(unsigned Opcode, const User &U);

  // (Elementwise) Integer Multiplication / Division
  void translateUDiv(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VUDIV, U);
    else
      translateBinaryOp(SWPP::UDIV, U);
  }
  void translateSDiv(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VSDIV, U);
    else
      translateBinaryOp(SWPP::SDIV, U);
  }
  void translateURem(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VUREM, U);
    else
      translateBinaryOp(SWPP::UREM, U);
  }
  void translateSRem(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VSREM, U);
    else
      translateBinaryOp(SWPP::SREM, U);
  }
  void translateMul(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VMUL, U);
    else
      translateBinaryOp(SWPP::MUL, U);
  }

  // (Elementwise) Integer Shift
  void translateShl(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VSHL, U);
    else
      translateBinaryOp(SWPP::SHL, U);
  }
  void translateLShr(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VLSHR, U);
    else
      translateBinaryOp(SWPP::LSHR, U);
  }
  void translateAShr(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VASHR, U);
    else
      translateBinaryOp(SWPP::ASHR, U);
  }

  // (Elementwise) Bitwise Operation
  void translateAnd(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VAND, U);
    else
      translateBinaryOp(SWPP::AND, U);
  }
  void translateOr(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VOR, U);
    else
      translateBinaryOp(SWPP::OR, U);
  }
  void translateXor(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VXOR, U);
    else
      translateBinaryOp(SWPP::XOR, U);
  }

  // (Elementwise) Integer Add / Sub
  void translateAdd(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VADD, U);
    else
      translateBinaryOp(SWPP::EADD, U);
  }
  void translateOAdd(const User &U) {
      translateBinaryOp(SWPP::OADD, U);
  }
  void translateSub(const User &U) {
    if (isVector(U))
      translateBinaryOp(SWPP::VSUB, U);
    else
      translateBinaryOp(SWPP::ESUB, U);
  }
  void translateOSub(const User &U) {
      translateBinaryOp(SWPP::OSUB, U);
  }

  // (Elementwise) Integer Increment / Decrement
  void translateIncr(const User &U) {
    if (isVector(U))
      translateUnaryOp(SWPP::VINCR, U);
    else
      translateUnaryOp(SWPP::INCR, U);
  }
  void translateDecr(const User &U) {
    if (isVector(U))
      translateUnaryOp(SWPP::VDECR, U);
    else
      translateUnaryOp(SWPP::DECR, U);
  }

  // Comparison Base
  void translateICmpBase(unsigned Opcode, const User &U);

  // (Elementwise) Comparison
  void translateICmp(const User &U) {
    if (isVector(U))
      translateICmpBase(SWPP::VICMP, U);
    else
      translateICmpBase(SWPP::ICMP, U);
  }

  // Ternary Base
  void translateSelectBase(unsigned Opcode, const User &U);

  // (Elementwise) Ternary
  void translateSelect(const User &U) {
    if (isVector(U))
      translateSelectBase(SWPP::VSELECT, U);
    else
      translateSelectBase(SWPP::SELECT, U);
  }

  // Parallel Integer Multiplication / Division
  void translateVPUDiv(const User &U) {
    isVector(U);
    translateBinaryOp(SWPP::VPUDIV, U);
  }
  void translateVPSDiv(const User &U) {
    isVector(U);
    translateBinaryOp(SWPP::VPSDIV, U);
  }
  void translateVPURem(const User &U) {
    isVector(U);
    translateBinaryOp(SWPP::VPUREM, U);
  }
  void translateVPSRem(const User &U) {
    isVector(U);
    translateBinaryOp(SWPP::VPSREM, U);
  }
  void translateVPMul(const User &U) {
    isVector(U);
    translateBinaryOp(SWPP::VPMUL, U);
  }

  // Parallel Bitwise Operation
  void translateVPAnd(const User &U) {
    isVector(U);
    translateBinaryOp(SWPP::VPAND, U);
  }
  void translateVPOr(const User &U) {
    isVector(U);
    translateBinaryOp(SWPP::VPOR, U);
  }
  void translateVPXor(const User &U) {
    isVector(U);
    translateBinaryOp(SWPP::VPXOR, U);
  }

  // Parallel Integer Add / Sub
  void translateVPAdd(const User &U) {
    isVector(U);
    translateBinaryOp(SWPP::VPADD, U);
  }
  void translateVPSub(const User &U) {
    isVector(U);
    translateBinaryOp(SWPP::VPSUB, U);
  }

  // Parallel Comparision
  void translateVPICmp(const User &U) {
    isVector(U);
    translateICmpBase(SWPP::VPICMP, U);
  }

  // Parallel Ternary
  void translateVPSelect(const User &U) {
    isVector(U);
    translateSelectBase(SWPP::VPSELECT, U);
  }

  // Vector Manipulation
  void translateBroadcastElement(const User &U);
  void translateInsertElement(const User &U);
  void translateExtractElement(const User &U);

  // Copy
  void translateCopy(const User &U);

  // Helper class for building MachineInstr
  std::unique_ptr<MachineIRBuilder> CurBuilder;

  void finalizeFunction();

  // if Value is constant, then get immediate(int64_t)
  // else, find VMap for virtual register. if not exists then create
  std::variant<Register, int64_t> getOrCreateVRegOrImm(const Value &V);

  // Src can be either register or immediate
  SrcOp getSrc(const Value &V) {
    return std::visit([](auto &&arg) -> SrcOp { return SrcOp(arg); },
                      getOrCreateVRegOrImm(V));
  }

  // Dst must be register
  Register getDst(const Value &V) {
    return std::get<Register>(getOrCreateVRegOrImm(V));
  }

  MachineBasicBlock &getMBB(const BasicBlock &BB) { return *BBToMBB[&BB]; }

public:
  bool run(MachineFunction &);

  MIRTranslatorImpl();
};

class MIRTranslator : public PassInfoMixin<MIRTranslator> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM) {
    MIRTranslatorImpl Impl;
    bool changed = Impl.run(MF);
    return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

class MIRTranslatorWrapperPass : public MachineFunctionPass {
public:
  static char ID;

  MIRTranslatorWrapperPass() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    MIRTranslatorImpl Impl;
    return Impl.run(MF);
  }
};

} // namespace sc::backend::mirtranslator

#endif
