#include "mir_translator.h"
#include "SWPP/SWPPInstrInfo.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/MC/MCRegister.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

using namespace llvm;

namespace sc::backend::mirtranslator {

char MIRTranslatorWrapperPass::ID = 0;

MIRTranslatorImpl::MIRTranslatorImpl()
    : PreCallHandlers([this] {
        std::unordered_map<std::string, PreCallHandler> Handlers;
        static constexpr std::array<const char *, 5> ScalarWidths = {
            "1", "8", "16", "32", "64"};
        static constexpr std::array<const char *, 4> StoreWidths = {"8", "16",
                                                                    "32", "64"};
        static constexpr std::array<const char *, 2> VectorWidths = {"32x8",
                                                                     "64x4"};

        auto addHandler = [&](std::string Name, PreCallHandler Handler) {
          Handlers.emplace(std::move(Name), std::move(Handler));
        };
        auto addScalarOverloads = [&](const char *Prefix,
                                      PreCallHandler Handler) {
          for (const char *Width : ScalarWidths)
            addHandler(std::string(Prefix) + Width, Handler);
        };
        auto addMemOverloads = [&](const char *Prefix, PreCallHandler Handler) {
          for (const char *Width : StoreWidths)
            addHandler(std::string(Prefix) + Width, Handler);
        };
        auto addVectorOverloads = [&](const char *Prefix,
                                      PreCallHandler Handler) {
          for (const char *Width : VectorWidths)
            addHandler(std::string(Prefix) + Width, Handler);
        };

        addHandler("malloc", [this](const User &U) { translateMalloc(U); });
        addHandler("free", [this](const User &U) { translateFree(U); });
        addMemOverloads("aload_i",
                        [this](const User &U) { translateALoad(U); });
        addScalarOverloads("incr_i",
                           [this](const User &U) { translateIncr(U); });
        addScalarOverloads("decr_i",
                           [this](const User &U) { translateDecr(U); });
        addScalarOverloads("oadd_i",
                           [this](const User &U) { translateOAdd(U); });
        addScalarOverloads("osub_i",
                           [this](const User &U) { translateOSub(U); });
        addVectorOverloads("vpudiv_i",
                           [this](const User &U) { translateVPUDiv(U); });
        addVectorOverloads("vpsdiv_i",
                           [this](const User &U) { translateVPSDiv(U); });
        addVectorOverloads("vpurem_i",
                           [this](const User &U) { translateVPURem(U); });
        addVectorOverloads("vpsrem_i",
                           [this](const User &U) { translateVPSRem(U); });
        addVectorOverloads("vpmul_i",
                           [this](const User &U) { translateVPMul(U); });
        addVectorOverloads("vpand_i",
                           [this](const User &U) { translateVPAnd(U); });
        addVectorOverloads("vpor_i",
                           [this](const User &U) { translateVPOr(U); });
        addVectorOverloads("vpxor_i",
                           [this](const User &U) { translateVPXor(U); });
        addVectorOverloads("vpadd_i",
                           [this](const User &U) { translateVPAdd(U); });
        addVectorOverloads("vpsub_i",
                           [this](const User &U) { translateVPSub(U); });
        addVectorOverloads("vpicmp_i",
                           [this](const User &U) { translateVPICmp(U); });
        addVectorOverloads("vpselect_i",
                           [this](const User &U) { translateVPSelect(U); });
        addVectorOverloads("vbcast_i", [this](const User &U) {
          translateBroadcastElement(U);
        });

        return Handlers;
      }()) {}

bool MIRTranslatorImpl::isVector(const Value &V) {
  auto *Ty = V.getType();
  if (auto *VectorTy = dyn_cast<VectorType>(Ty)) {
    auto *FixedVectorTy = dyn_cast<FixedVectorType>(VectorTy);
    if (FixedVectorTy == nullptr)
      report_fatal_error("Not supported vector type");
    Type *ElementTy = FixedVectorTy->getElementType();
    unsigned NumElements = FixedVectorTy->getNumElements();
    if ((NumElements != 8 || !ElementTy->isIntegerTy(32)) &&
        (NumElements != 4 || !ElementTy->isIntegerTy(64)))
      report_fatal_error("Not supported vector type");
    return true;
  }
  if (Ty->isPointerTy())
    return false;
  if (auto *IntTy = dyn_cast<IntegerType>(Ty)) {
    unsigned bw = IntTy->getBitWidth();
    if (bw == 1 || bw == 8 || bw == 16 || bw == 32 || bw == 64)
      return false;
  }
  report_fatal_error("Not supported scalar type");
}

// This function checks if a given LLVM IR Value is a constant integer, null
// pointer, or a register. If it's not a register, it creates a new virtual
// register for it.
std::variant<Register, int64_t>
MIRTranslatorImpl::getOrCreateVRegOrImm(const Value &V) {
  if (isa<Constant>(V)) {
    // Check if the value is a constant integer (immediate value)
    if (const auto *CI = dyn_cast<ConstantInt>(&V)) {
      if (CI->getBitWidth() == 1) {
        if (CI->isZero())
          return 0;
        return 1;
      }
      return CI->getValue().getSExtValue();
    }
    // Check if the value is a undef or poison
    if (isa<UndefValue>(&V)) {
      Register VReg;
      if (isVector(V))
        VReg = MRI->createVirtualRegister(TRI->getRegClass(1));
      else
        VReg = MRI->createVirtualRegister(TRI->getRegClass(0));
      UndefRegs.push_back(VReg);
      return VReg;
    }
    // Check if the value is a null pointer
    if (isa<ConstantPointerNull>(&V))
      return 153600;
    // BlockAddress, ConstantAggregate, ConstantExpr, GlobalValue, ConstantFP
    // ...
    report_fatal_error("Not supported Constant Value");
  }

  // Check value is supported in SWPP target machine
  isVector(V);
  // Check if the value already has an associated virtual register
  if (VMap.contains(V))
    return VMap.findVReg(V);

  // Create a new virtual register for the value
  Register &VReg = VMap.getVReg(V);
  if (V.getType()->isVectorTy())
    VReg = MRI->createVirtualRegister(TRI->getRegClass(1)); // VecRegClass
  else
    VReg = MRI->createVirtualRegister(TRI->getRegClass(0)); // GenRegClass

  return VReg;
}

// ret Val
void MIRTranslatorImpl::translateRet(const User &U) {
  const auto &RI = cast<ReturnInst>(U);
  const Value *Ret = RI.getReturnValue();
  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::RET);
  if (Ret == nullptr) {
    MIB.addImm(0); // ret always returns a value, add dummy operand
  } else {
    SrcOp Val = getSrc(*Ret);
    Val.addSrcToMIB(MIB);
  }
}

// (uncond) br Succ0MBB
// (cond)   br Cond SuccMBB0 SuccMBB1
void MIRTranslatorImpl::translateBr(const User &U) {
  const auto &BrInst = cast<BranchInst>(U);
  auto &CurMBB = CurBuilder->getMBB();
  auto *SuccMBB0 = &getMBB(*BrInst.getSuccessor(0));
  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::BR);

  if (BrInst.isUnconditional()) {
    MIB.addMBB(SuccMBB0);
    CurMBB.addSuccessor(SuccMBB0);
    return;
  }

  SrcOp Cond = getSrc(*BrInst.getCondition());
  MachineBasicBlock *SuccMBB1 = &getMBB(*BrInst.getSuccessor(1));
  Cond.addSrcToMIB(MIB);
  MIB.addMBB(SuccMBB0).addMBB(SuccMBB1);
  CurMBB.addSuccessor(SuccMBB0);
  CurMBB.addSuccessor(SuccMBB1);
}

// switch Cond CaseValue1 SuccMBB1 ... CaseValueN SuccMBBN DefaultMBB
void MIRTranslatorImpl::translateSwitch(const User &U) {
  const auto &SwInst = cast<SwitchInst>(U);
  auto &CurMBB = CurBuilder->getMBB();
  SrcOp Cond = getSrc(*SwInst.getCondition());
  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::SWITCH);
  Cond.addSrcToMIB(MIB);

  for (const auto &Case : SwInst.cases()) {
    const ConstantInt *CaseValue = Case.getCaseValue();
    const BasicBlock *CaseSucc = Case.getCaseSuccessor();
    MachineBasicBlock *SuccMBB = &getMBB(*CaseSucc);
    MIB.addImm(CaseValue->getSExtValue());
    MIB.addMBB(SuccMBB);
    CurMBB.addSuccessor(SuccMBB);
  }

  MachineBasicBlock *DefaultMBB = &getMBB(*SwInst.getDefaultDest());
  MIB.addMBB(DefaultMBB);
  CurMBB.addSuccessor(DefaultMBB);
}

// (void) call FName Arg1 ... ArgN
// (else) Dst = call FName Arg1 ... ArgN
void MIRTranslatorImpl::translateCall(const User &U) {
  const auto &CI = cast<CallInst>(U);
  const Function *F = CI.getCalledFunction();
  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::CALL);

  if (!CI.getType()->isVoidTy()) {
    Register Dst = getDst(CI);
    MIB.addDef(Dst);
  }

  // Add function name by external symbol
  MIB.addExternalSymbol(F->getName().data());

  // Add arguments
  if (CI.arg_size() > 16)
    report_fatal_error("The maximum number of arguments is 16");
  for (const auto &Arg : CI.args()) {
    SrcOp Src = getSrc(*dyn_cast<Value>(&Arg));
    Src.addSrcToMIB(MIB);
  }
}

// (void) rcall Arg1 ... ArgN
// (else) Dst = rcall Arg1 ... ArgN
void MIRTranslatorImpl::translateRCall(const User &U) {
  const auto &CI = cast<CallInst>(U);
  const Function *PF = CI.getFunction();
  const Function *F = CI.getCalledFunction();

  if (PF != F)
    report_fatal_error("RCall must call the same function");

  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::RCALL);

  if (!CI.getType()->isVoidTy()) {
    Register Dst = getDst(CI);
    MIB.addDef(Dst);
  }

  for (const auto &Arg : CI.args()) {
    SrcOp Src = getSrc(*dyn_cast<Value>(&Arg));
    Src.addSrcToMIB(MIB);
  }
}

// Reg = phi
// It defers the processing of PHI nodes until all basic blocks have been
// translated, since PHI nodes refer to values from predecessor blocks that
// might not be available yet.
void MIRTranslatorImpl::translatePHI(const User &U) {
  const auto &PI = cast<PHINode>(U);
  Register Reg = getDst(PI);
  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::PHI);
  MIB.addDef(Reg);
  PendingPHIs.push_back({&PI, MIB.getInstr()});
}

// Reg = phi ValReg1 Pred1 ... ValRegN PredN
// This function completes the translation of PHI nodes by adding
// the incoming values and predecessor basic blocks after all other
// instructions have been processed.
void MIRTranslatorImpl::finishPendingPhis() {
  for (auto &Phi : PendingPHIs) {
    const PHINode *PI = Phi.first;
    MachineInstr *ComponentPHI = Phi.second;
    SmallSet<const MachineBasicBlock *, 16> SeenPreds;

    for (unsigned i = 0; i < PI->getNumIncomingValues(); ++i) {
      auto *IRPred = PI->getIncomingBlock(i);
      Value *Val = PI->getIncomingValue(i);
      std::variant<Register, int64_t> MVal = getOrCreateVRegOrImm(*Val);
      MachineBasicBlock *Pred = &getMBB(*IRPred);

      if (SeenPreds.count(Pred) > 0)
        continue;

      SeenPreds.insert(Pred);
      Register ValReg;

      if (std::holds_alternative<int64_t>(MVal)) {
        // Convert immediate values to virtual registers for LiveVariables and
        // PHIElimination
        int64_t ImmVal = std::get<int64_t>(MVal);
        ValReg = MRI->createVirtualRegister(TRI->getRegClass(0));
        BuildMI(*Pred, Pred->getFirstTerminator(), nullptr,
                TII->get(SWPP::CONST), ValReg)
            .addImm(ImmVal);
      } else if (std::holds_alternative<Register>(MVal))
        ValReg = std::get<Register>(MVal);

      MachineInstrBuilder MIB(*MF, ComponentPHI);
      MIB.addUse(ValReg);
      MIB.addMBB(Pred);
    }
  }
}

// Ptr = malloc Size
void MIRTranslatorImpl::translateMalloc(const User &U) {
  const auto &CI = cast<CallInst>(U);
  SrcOp Size = getSrc(*CI.getOperand(0));
  Register Ptr = getDst(U);
  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::MALLOC);
  MIB.addDef(Ptr);
  Size.addSrcToMIB(MIB);
}

// Initially, the result is represented by a `CONST` instruction with a frame
// index. After the frame layout is finalized, this `CONST` is typically
// replaced with an `ADD` instruction that calculates the stack pointer offset.
// Example:
// - Initial: `Ptr = const %stack.idx`
// - Final: `Ptr = add sp, offset`
void MIRTranslatorImpl::translateAlloca(const User &U) {
  const auto &AI = cast<AllocaInst>(U);

  if (!isa<ConstantInt>(AI.getArraySize()))
    report_fatal_error("Dynamic Alloca is not supported");

  Register Ptr = getDst(AI);
  uint64_t ElementSize = DL->getTypeAllocSize(AI.getAllocatedType());
  uint64_t Size =
      ElementSize * cast<ConstantInt>(AI.getArraySize())->getZExtValue();

  // Create stack object in MachineFrameInfo
  int idx = MF->getFrameInfo().CreateStackObject(Size, Align(1), false, &AI);
  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::CONST);
  MIB.addDef(Ptr);
  MIB.addFrameIndex(idx);
}

// free Ptr
void MIRTranslatorImpl::translateFree(const User &U) {
  const auto &CI = cast<CallInst>(U);
  SrcOp Ptr = getSrc(*CI.getOperand(0));
  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::FREE);
  Ptr.addSrcToMIB(MIB);
}

// Reg = aload LoadSize Ptr
void MIRTranslatorImpl::translateALoad(const User &U) {
  const auto &CI = cast<CallInst>(U);
  auto LoadSize = static_cast<int64_t>(DL->getTypeStoreSize(CI.getType()));
  SrcOp Ptr = getSrc(*CI.getOperand(0));
  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::ALOAD);
  MIB.addDef(getDst(CI));
  MIB.addImm(LoadSize);
  Ptr.addSrcToMIB(MIB);
}

// Reg = load LoadSize Ptr
// Reg = vload Ptr
void MIRTranslatorImpl::translateLoad(const User &U) {
  const auto &LI = cast<LoadInst>(U);
  Register Reg = getDst(LI);
  SrcOp Ptr = getSrc(*LI.getPointerOperand());
  auto LoadSize = static_cast<int64_t>(DL->getTypeStoreSize(LI.getType()));
  unsigned Opc = SWPP::LOAD;

  if (isVector(LI))
    Opc = SWPP::VLOAD;

  MachineInstrBuilder MIB = CurBuilder->buildInstr(Opc);
  MIB.addDef(Reg);

  if (Opc == SWPP::LOAD)
    MIB.addImm(LoadSize);

  Ptr.addSrcToMIB(MIB);
}

// store StoreSize Reg Ptr
// vstore Reg Ptr
void MIRTranslatorImpl::translateStore(const User &U) {
  const auto &SI = cast<StoreInst>(U);
  SrcOp Reg = getSrc(*SI.getValueOperand());
  SrcOp Ptr = getSrc(*SI.getPointerOperand());
  auto StoreSize = static_cast<int64_t>(
      DL->getTypeStoreSize(SI.getValueOperand()->getType()));
  unsigned Opc = SWPP::STORE;

  if (isVector(*SI.getValueOperand()))
    Opc = SWPP::VSTORE;

  MachineInstrBuilder MIB = CurBuilder->buildInstr(Opc);

  if (Opc == SWPP::STORE)
    MIB.addImm(StoreSize);

  Reg.addSrcToMIB(MIB);
  Ptr.addSrcToMIB(MIB);
}

// Res = Opcode Src0 Size
void MIRTranslatorImpl::translateUnaryOp(unsigned Opcode, const User &U) {
  SrcOp Src0 = getSrc(*U.getOperand(0));
  Register Res = getDst(U);
  auto Size =
      static_cast<int64_t>(DL->getTypeSizeInBits(U.getType()->getScalarType()));
  MachineInstrBuilder MIB = CurBuilder->buildInstr(Opcode);
  MIB.addDef(Res);
  Src0.addSrcToMIB(MIB);
  MIB.addImm(Size);
}

// Res = Opcode Src0 Src1 Size
void MIRTranslatorImpl::translateBinaryOp(unsigned Opcode, const User &U) {
  SrcOp Src0 = getSrc(*U.getOperand(0));
  SrcOp Src1 = getSrc(*U.getOperand(1));
  Register Res = getDst(U);
  auto Size =
      static_cast<int64_t>(DL->getTypeSizeInBits(U.getType()->getScalarType()));

  MachineInstrBuilder MIB = CurBuilder->buildInstr(Opcode);
  MIB.addDef(Res);
  Src0.addSrcToMIB(MIB);
  Src1.addSrcToMIB(MIB);
  MIB.addImm(Size);
}

// Res = (v)(p)icmp Pred Src0 Src1 Size
void MIRTranslatorImpl::translateICmpBase(unsigned Opcode, const User &U) {
  const auto &CI = cast<ICmpInst>(U);
  SrcOp Pred = CI.getPredicate();
  SrcOp Src0 = getSrc(*U.getOperand(0));
  SrcOp Src1 = getSrc(*U.getOperand(1));
  Register Res = getDst(U);
  auto Size = static_cast<int64_t>(
      DL->getTypeSizeInBits(CI.getOperand(0)->getType()->getScalarType()));

  MachineInstrBuilder MIB = CurBuilder->buildInstr(Opcode);
  MIB.addDef(Res);
  Pred.addSrcToMIB(MIB);
  Src0.addSrcToMIB(MIB);
  Src1.addSrcToMIB(MIB);
  MIB.addImm(Size);
}

// Res = (v)(p)select Tst Op0 Op1 Size
void MIRTranslatorImpl::translateSelectBase(unsigned Opcode, const User &U) {
  SrcOp Tst = getSrc(*U.getOperand(0));
  Register Res = getDst(U);
  SrcOp Op0 = getSrc(*U.getOperand(1));
  SrcOp Op1 = getSrc(*U.getOperand(2));
  auto Size =
      static_cast<int64_t>(DL->getTypeSizeInBits(U.getType()->getScalarType()));

  MachineInstrBuilder MIB = CurBuilder->buildInstr(Opcode);
  MIB.addDef(Res);
  Tst.addSrcToMIB(MIB);
  Op0.addSrcToMIB(MIB);
  Op1.addSrcToMIB(MIB);
  MIB.addImm(Size);
}

// Res = vbcast Val Size
void MIRTranslatorImpl::translateBroadcastElement(const User &U) {
  Register Res = getDst(U);
  SrcOp Val = getSrc(*U.getOperand(0));
  auto Size =
      static_cast<int64_t>(DL->getTypeSizeInBits(U.getType()->getScalarType()));

  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::VBCAST);
  MIB.addDef(Res);
  Val.addSrcToMIB(MIB);
  MIB.addImm(Size);
}

// Res = vupdate Val Elt Idx Size
void MIRTranslatorImpl::translateInsertElement(const User &U) {
  Register Res = getDst(U);
  SrcOp Val = getSrc(*U.getOperand(0));
  SrcOp Elt = getSrc(*U.getOperand(1));
  SrcOp Idx = getSrc(*U.getOperand(2));
  auto Size =
      static_cast<int64_t>(DL->getTypeSizeInBits(U.getType()->getScalarType()));

  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::VUPDATE);
  MIB.addDef(Res);
  Val.addSrcToMIB(MIB);
  Elt.addSrcToMIB(MIB);
  Idx.addSrcToMIB(MIB);
  MIB.addImm(Size);
}

// Res = VEXTCT Val Idx Size
void MIRTranslatorImpl::translateExtractElement(const User &U) {
  Register Res = getDst(U);
  SrcOp Val = getSrc(*U.getOperand(0));
  SrcOp Idx = getSrc(*U.getOperand(1));
  auto Size =
      static_cast<int64_t>(DL->getTypeSizeInBits(U.getType()->getScalarType()));

  MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::VEXTCT);
  MIB.addDef(Res);
  Val.addSrcToMIB(MIB);
  Idx.addSrcToMIB(MIB);
  MIB.addImm(Size);
}

// This function identifies and translates specific SWPP intrinsic function
// calls into corresponding Machine IR instructions. If the function is not a
// recognized intrinsic, it falls back to translating it as a general function
// call.
void MIRTranslatorImpl::translatePreCall(const User &U) {
  const auto &CI = cast<CallInst>(U);
  const Function *CalledFunc = CI.getCalledFunction();
  if (CalledFunc == nullptr)
    return translateCall(U);

  auto It = PreCallHandlers.find(CalledFunc->getName().str());
  if (It != PreCallHandlers.end())
    return It->second(U);

  // if (CI.isMustTailCall())
  //   return translateRCall(U);

  return translateCall(U);
}

// Res = const ImmVal
// Res = copy RegVal
void MIRTranslatorImpl::translateCopy(const User &U) {
  Register Res = getDst(U);
  std::variant<Register, int64_t> Val = getOrCreateVRegOrImm(*U.getOperand(0));
  // In the register coalescer, both the source and destination of a COPY
  // must be registers. Therefore, if the value is an immediate, we use
  // a CONST instruction to create a virtual register.
  if (std::holds_alternative<int64_t>(Val)) {
    int64_t ImmVal = std::get<int64_t>(Val);
    MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::CONST);
    MIB.addDef(Res);
    MIB.addImm(ImmVal);
  } else if (std::holds_alternative<Register>(Val)) {
    Register RegVal = std::get<Register>(Val);
    MachineInstrBuilder MIB = CurBuilder->buildInstr(SWPP::COPY);
    MIB.addDef(Res);
    MIB.addUse(RegVal);
  } else
    llvm_unreachable("Val should be either Register or int64_t");
}

void MIRTranslatorImpl::translate(const Instruction &Inst) {
  if (Inst.isCast()) {
    translateCopy(Inst);
    return;
  }
  switch (Inst.getOpcode()) {
  case Instruction::Ret:
    translateRet(Inst);
    break;
  case Instruction::Br:
    translateBr(Inst);
    break;
  case Instruction::Switch:
    translateSwitch(Inst);
    break;
  case Instruction::Load:
    translateLoad(Inst);
    break;
  case Instruction::Store:
    translateStore(Inst);
    break;
  case Instruction::UDiv:
    translateUDiv(Inst);
    break;
  case Instruction::SDiv:
    translateSDiv(Inst);
    break;
  case Instruction::URem:
    translateURem(Inst);
    break;
  case Instruction::SRem:
    translateSRem(Inst);
    break;
  case Instruction::Mul:
    translateMul(Inst);
    break;
  case Instruction::Shl:
    translateShl(Inst);
    break;
  case Instruction::LShr:
    translateLShr(Inst);
    break;
  case Instruction::AShr:
    translateAShr(Inst);
    break;
  case Instruction::And:
    translateAnd(Inst);
    break;
  case Instruction::Or:
    translateOr(Inst);
    break;
  case Instruction::Xor:
    translateXor(Inst);
    break;
  case Instruction::Add:
    translateAdd(Inst);
    break;
  case Instruction::Sub:
    translateSub(Inst);
    break;
  case Instruction::ICmp:
    translateICmp(Inst);
    break;
  case Instruction::Select:
    translateSelect(Inst);
    break;
  case Instruction::ExtractElement:
    translateExtractElement(Inst);
    break;
  case Instruction::InsertElement:
    translateInsertElement(Inst);
    break;
  case Instruction::Alloca:
    translateAlloca(Inst);
    break;
  case Instruction::PHI:
    translatePHI(Inst);
    break;
  case Instruction::Freeze:
    translateCopy(Inst);
    break;
  case Instruction::Call:
    translatePreCall(Inst);
    break;
  case Instruction::GetElementPtr:
  default:
    // IndirectBr, Invoke, Resume, Unreachable, CleanupRet, CatchRet,
    // CatchSwitch, CallBr, FNeg, FAdd, FSub, FMul, FDiv, FRem, Fence,
    // AtomicCmpXchg, AtomicRMW, CleanupPad, CatchPad, FCmp, VAArg,
    // ShuffleVector, ExtractValue, InsertValue, LandingPad
    report_fatal_error("Instruction Not Supported");
  }
}

bool MIRTranslatorImpl::run(llvm::MachineFunction &CurMF) {
  MF = &CurMF;
  F = &MF->getFunction();

  // Skip translation if the function is only a declaration.
  if (F->isDeclaration())
    return false;

  // Initialize the Machine IR Builder and register information.
  CurBuilder = std::make_unique<MachineIRBuilder>();
  CurBuilder->setMF(*MF);
  MRI = &MF->getRegInfo();
  TRI = MF->getSubtarget().getRegisterInfo();
  TII = MF->getSubtarget().getInstrInfo();
  DL = &F->getParent()->getDataLayout();

  // Create a MachineBasicBlock for each BasicBlock in the function.
  for (const auto &BB : *F) {
    auto *&MBB = BBToMBB[&BB];
    MBB = MF->CreateMachineBasicBlock(&BB);
    MF->push_back(MBB);
  }

  // Handle function arguments, creating virtual registers for them.
  SmallVector<std::pair<Register, MCRegister>, 4> VRegArgs;
  if (F->arg_size() > 16)
    report_fatal_error("The maximum number of arguments is 16");
  for (unsigned i = 0; i < F->arg_size(); i++) {
    MCRegister MCReg = 2 + i;
    Register VReg = MF->addLiveIn(MCReg, TRI->getRegClass(0));
    VMap.getVReg(*F->getArg(i)) = VReg;
    VRegArgs.push_back({VReg, MCReg});
  }

  // Freeze argument registers to prevent their use in the register allocator.
  MF->getRegInfo().freezeReservedRegs();

  // Translate each instruction in reverse post-order (RPO) to ensure PHI nodes
  // are handled correctly.
  ReversePostOrderTraversal<const Function *> RPOT(F);
  for (const auto *BB : RPOT) {
    MachineBasicBlock &MBB = getMBB(*BB);
    CurBuilder->setMBB(MBB);
    for (const Instruction &Inst : *BB)
      translate(Inst);
  }

  // Finish translating any pending PHI nodes.
  finishPendingPhis();

  // Insert COPY instructions for function arguments.
  MachineBasicBlock *entry = &getMBB(F->getEntryBlock());
  MachineBasicBlock::iterator I = entry->getFirstNonPHI();
  for (auto &VRegArg : VRegArgs) {
    BuildMI(*entry, I, nullptr, TII->get(SWPP::COPY), VRegArg.first)
        .addUse(VRegArg.second);
  }

  // Insert IMPLICIT_DEF into entry block (Undef register need def)
  for (auto Reg : UndefRegs)
    BuildMI(*entry, I, nullptr, TII->get(SWPP::IMPLICIT_DEF), Reg);

  // Clear temporary data structures used during translation.
  PendingPHIs.clear();
  VMap.reset();
  BBToMBB.clear();
  CurBuilder.reset();
  return true;
}

} // namespace sc::backend::mirtranslator
