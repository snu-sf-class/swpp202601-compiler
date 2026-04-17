#ifndef SC_BACKEND_SWPP_INSTRINFO_H
#define SC_BACKEND_SWPP_INSTRINFO_H

#include "SWPPRegisterInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include <array>

namespace llvm {

namespace SWPP {
enum {
  PHI = TargetOpcode::PHI,
  INLINEASM = TargetOpcode::INLINEASM,
  INLINEASM_BR = TargetOpcode::INLINEASM_BR,
  CFI_INSTRUCTION = TargetOpcode::CFI_INSTRUCTION,
  EH_LABEL = TargetOpcode::EH_LABEL,
  GC_LABEL = TargetOpcode::GC_LABEL,
  ANNOTATION_LABEL = TargetOpcode::ANNOTATION_LABEL,
  KILL = TargetOpcode::KILL,
  EXTRACT_SUBREG = TargetOpcode::EXTRACT_SUBREG,
  INSERT_SUBREG = TargetOpcode::INSERT_SUBREG,
  IMPLICIT_DEF = TargetOpcode::IMPLICIT_DEF,
  SUBREG_TO_REG = TargetOpcode::SUBREG_TO_REG,
  COPY_TO_REGCLASS = TargetOpcode::COPY_TO_REGCLASS,
  DBG_VALUE = TargetOpcode::DBG_VALUE,
  DBG_VALUE_LIST = TargetOpcode::DBG_VALUE_LIST,
  DBG_INSTR_REF = TargetOpcode::DBG_INSTR_REF,
  DBG_PHI = TargetOpcode::DBG_PHI,
  DBG_LABEL = TargetOpcode::DBG_LABEL,
  REG_SEQUENCE = TargetOpcode::REG_SEQUENCE,
  COPY = TargetOpcode::COPY,
  BUNDLE = TargetOpcode::BUNDLE,
  LIFETIME_START = TargetOpcode::LIFETIME_START,
  LIFETIME_END = TargetOpcode::LIFETIME_END,
  PSEUDO_PROBE = TargetOpcode::PSEUDO_PROBE,
  ARITH_FENCE = TargetOpcode::ARITH_FENCE,
  STACKMAP = TargetOpcode::STACKMAP,
  FENTRY_CALL = TargetOpcode::FENTRY_CALL,
  PATCHPOINT = TargetOpcode::PATCHPOINT,
  LOAD_STACK_GUARD = TargetOpcode::LOAD_STACK_GUARD,
  PREALLOCATED_SETUP = TargetOpcode::PREALLOCATED_SETUP,
  PREALLOCATED_ARG = TargetOpcode::PREALLOCATED_ARG,
  STATEPOINT = TargetOpcode::STATEPOINT,
  LOCAL_ESCAPE = TargetOpcode::LOCAL_ESCAPE,
  FAULTING_OP = TargetOpcode::FAULTING_OP,
  PATCHABLE_OP = TargetOpcode::PATCHABLE_OP,
  PATCHABLE_FUNCTION_ENTER = TargetOpcode::PATCHABLE_FUNCTION_ENTER,
  PATCHABLE_RET = TargetOpcode::PATCHABLE_RET,
  PATCHABLE_FUNCTION_EXIT = TargetOpcode::PATCHABLE_FUNCTION_EXIT,
  PATCHABLE_TAIL_CALL = TargetOpcode::PATCHABLE_TAIL_CALL,
  PATCHABLE_EVENT_CALL = TargetOpcode::PATCHABLE_EVENT_CALL,
  PATCHABLE_TYPED_EVENT_CALL = TargetOpcode::PATCHABLE_TYPED_EVENT_CALL,
  ICALL_BRANCH_FUNNEL = TargetOpcode::ICALL_BRANCH_FUNNEL,
  MEMBARRIER = TargetOpcode::MEMBARRIER,
  JUMP_TABLE_DEBUG_INFO = TargetOpcode::JUMP_TABLE_DEBUG_INFO,
  RET = TargetOpcode::PRE_ISEL_GENERIC_OPCODE_END + 1,
  BR,
  SWITCH,
  CALL,
  RCALL,
  MALLOC,
  FREE,
  LOAD,
  ALOAD,
  STORE,
  VLOAD,
  VSTORE,
  UDIV,
  SDIV,
  UREM,
  SREM,
  MUL,
  SHL,
  LSHR,
  ASHR,
  AND,
  OR,
  XOR,
  ADD,
  EADD,
  OADD,
  SUB,
  ESUB,
  OSUB,
  INCR,
  DECR,
  ICMP,
  SELECT,
  VUDIV,
  VSDIV,
  VUREM,
  VSREM,
  VMUL,
  VSHL,
  VLSHR,
  VASHR,
  VAND,
  VOR,
  VXOR,
  VADD,
  VSUB,
  VINCR,
  VDECR,
  VICMP,
  VSELECT,
  VPUDIV,
  VPSDIV,
  VPUREM,
  VPSREM,
  VPMUL,
  VPAND,
  VPOR,
  VPXOR,
  VPADD,
  VPSUB,
  VPICMP,
  VPSELECT,
  VBCAST,
  VEXTCT,
  VUPDATE,
  CONST,
  INSTRUCTION_LIST_END,
  G_PHI = TargetOpcode::G_PHI
};

} // end namespace SWPP

struct SWPPInstrTable {
  MCInstrDesc Insts[SWPP::INSTRUCTION_LIST_END];
  MCOperandInfo OperandInfo[20];
};

// { Opcode, NumOperands, NumDefs, 0, 0, 0, 0, OperandInfo offset, Flags, 0 }
// Flags: Variadic (Various NumOperands), VariadicOpsAreDefs (Various NumDefs)
// Barrier (BasicBlock must end with this instruction)
// OperandInfo[offset:offset+NumOperands] gives operand information of
// instruction
const SWPPInstrTable SWPPDescs = [] {
  SWPPInstrTable Table = {};
  auto reverseIndex = [](unsigned Opcode) {
    return SWPP::INSTRUCTION_LIST_END - 1 - Opcode;
  };
  auto setDesc = [&](const MCInstrDesc &Desc) {
    Table.Insts[reverseIndex(Desc.Opcode)] = Desc;
  };

  for (unsigned Opcode = 0; Opcode != SWPP::INSTRUCTION_LIST_END; ++Opcode)
    Table.Insts[reverseIndex(Opcode)] = {
        static_cast<unsigned short>(Opcode), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  setDesc({SWPP::PHI, 1, 1, 0, 0, 0, 0, 0, 0, 0 | (1ULL << MCID::Variadic), 0});
  setDesc({SWPP::IMPLICIT_DEF, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0});
  setDesc({SWPP::COPY, 2, 1, 0, 0, 0, 0, 0, 3, 0 | (1ULL << MCID::CheapAsAMove),
           0});
  setDesc({SWPP::LIFETIME_START, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0});
  setDesc({SWPP::LIFETIME_END, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0});

  setDesc({SWPP::RET, 0, 0, 0, 0, 0, 0, 0, 0,
           0 | (1ULL << MCID::Return) | (1ULL << MCID::Variadic), 0});
  setDesc({SWPP::BR, 1, 0, 0, 0, 0, 0, 0, 0,
           0 | (1ULL << MCID::Branch) | (1ULL << MCID::Variadic) |
               (1ULL << MCID::Terminator) | (1ULL << MCID::Barrier),
           0});
  setDesc({SWPP::SWITCH, 1, 0, 0, 0, 0, 0, 0, 0,
           0 | (1ULL << MCID::Branch) | (1ULL << MCID::Variadic) |
               (1ULL << MCID::Terminator) | (1ULL << MCID::Barrier),
           0});
  setDesc({SWPP::CALL, 1, 0, 0, 0, 0, 0, 0, 0,
           0 | (1ULL << MCID::Call) | (1ULL << MCID::Variadic) |
               (1ULL << MCID::VariadicOpsAreDefs),
           0});
  setDesc({SWPP::RCALL, 1, 0, 0, 0, 0, 0, 0, 0,
           0 | (1ULL << MCID::Call) | (1ULL << MCID::Variadic) |
               (1ULL << MCID::VariadicOpsAreDefs),
           0});
  setDesc({SWPP::MALLOC, 2, 1, 0, 0, 0, 0, 0, 1, 0, 0});
  setDesc({SWPP::FREE, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0});
  setDesc({SWPP::LOAD, 3, 1, 0, 0, 0, 0, 0, 3, 0 | (1ULL << MCID::MayLoad), 0});
  setDesc({SWPP::ALOAD, 3, 1, 0, 0, 0, 0, 0, 3, 0 | (1ULL << MCID::MayLoad), 0});
  setDesc(
      {SWPP::STORE, 3, 0, 0, 0, 0, 0, 0, 6, 0 | (1ULL << MCID::MayStore), 0});
  setDesc(
      {SWPP::VLOAD, 2, 1, 0, 0, 0, 0, 0, 3, 0 | (1ULL << MCID::MayLoad), 0});
  setDesc(
      {SWPP::VSTORE, 2, 0, 0, 0, 0, 0, 0, 6, 0 | (1ULL << MCID::MayStore), 0});
  setDesc({SWPP::UDIV, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::SDIV, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::UREM, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::SREM, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc(
      {SWPP::MUL, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc({SWPP::SHL, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::LSHR, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::ASHR, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc(
      {SWPP::AND, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::OR, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::XOR, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::ADD, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::EADD, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::OADD, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc({SWPP::SUB, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::ESUB, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::OSUB, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::INCR, 3, 1, 0, 0, 0, 0, 0, 3, 0, 0});
  setDesc({SWPP::DECR, 3, 1, 0, 0, 0, 0, 0, 3, 0, 0});
  setDesc({SWPP::ICMP, 5, 1, 0, 0, 0, 0, 0, 13, 0, 0});
  setDesc({SWPP::SELECT, 4, 1, 0, 0, 0, 0, 0, 13, 0, 0});
  setDesc({SWPP::VUDIV, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::VSDIV, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::VUREM, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::VSREM, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc(
      {SWPP::VMUL, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc({SWPP::VSHL, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::VLSHR, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::VASHR, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc(
      {SWPP::VAND, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::VOR, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::VXOR, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::VADD, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc({SWPP::VSUB, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::VINCR, 3, 1, 0, 0, 0, 0, 0, 3, 0, 0});
  setDesc({SWPP::VDECR, 3, 1, 0, 0, 0, 0, 0, 3, 0, 0});
  setDesc({SWPP::VICMP, 5, 1, 0, 0, 0, 0, 0, 13, 0, 0});
  setDesc({SWPP::VSELECT, 5, 1, 0, 0, 0, 0, 0, 13, 0, 0});
  setDesc({SWPP::VPUDIV, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::VPSDIV, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::VPUREM, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::VPSREM, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc(
      {SWPP::VPMUL, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::VPAND, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::VPOR, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::VPXOR, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc(
      {SWPP::VPADD, 4, 1, 0, 0, 0, 0, 0, 9, 0 | (1ULL << MCID::Commutable), 0});
  setDesc({SWPP::VPSUB, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::VPICMP, 5, 1, 0, 0, 0, 0, 0, 13, 0, 0});
  setDesc({SWPP::VPSELECT, 5, 1, 0, 0, 0, 0, 0, 13, 0, 0});
  setDesc({SWPP::VBCAST, 3, 1, 0, 0, 0, 0, 0, 3, 0, 0});
  setDesc({SWPP::VEXTCT, 4, 1, 0, 0, 0, 0, 0, 9, 0, 0});
  setDesc({SWPP::VUPDATE, 5, 1, 0, 0, 0, 0, 0, 13, 0, 0});
  setDesc({SWPP::CONST, 2, 1, 0, 0, 0, 0, 0, 3, 0, 0});

  const MCOperandInfo OperandInfo[] = {
      {-1, 0, MCOI::OPERAND_UNKNOWN, 0},   {-1, 0, MCOI::OPERAND_UNKNOWN, 0},
      {-1, 0, MCOI::OPERAND_UNKNOWN, 0},   {-1, 0, MCOI::OPERAND_REGISTER, 0},
      {-1, 0, MCOI::OPERAND_UNKNOWN, 0},   {-1, 0, MCOI::OPERAND_IMMEDIATE, 0},
      {-1, 0, MCOI::OPERAND_UNKNOWN, 0},   {-1, 0, MCOI::OPERAND_UNKNOWN, 0},
      {-1, 0, MCOI::OPERAND_IMMEDIATE, 0}, {-1, 0, MCOI::OPERAND_REGISTER, 0},
      {-1, 0, MCOI::OPERAND_UNKNOWN, 0},   {-1, 0, MCOI::OPERAND_UNKNOWN, 0},
      {-1, 0, MCOI::OPERAND_IMMEDIATE, 0}, {-1, 0, MCOI::OPERAND_REGISTER, 0},
      {-1, 0, MCOI::OPERAND_UNKNOWN, 0},   {-1, 0, MCOI::OPERAND_UNKNOWN, 0},
      {-1, 0, MCOI::OPERAND_UNKNOWN, 0},   {-1, 0, MCOI::OPERAND_IMMEDIATE, 0},
      {-1, 0, MCOI::OPERAND_REGISTER, 0},  {-1, 0, MCOI::OPERAND_IMMEDIATE, 0},
  };
  static_assert(sizeof(OperandInfo) / sizeof(OperandInfo[0]) ==
                    sizeof(Table.OperandInfo) / sizeof(Table.OperandInfo[0]),
                "Operand Info Num Incorrect");

  for (unsigned I = 0; I != sizeof(OperandInfo) / sizeof(OperandInfo[0]); ++I)
    Table.OperandInfo[I] = OperandInfo[I];

  return Table;
}();

// Instruction name (each instruction ends with \0)
const char SWPPInstrNameData[] = {"\0"
                                  "phi\0"
                                  "implicit_def\0"
                                  "copy\0"
                                  "ret\0"
                                  "br\0"
                                  "switch\0"
                                  "call\0"
                                  "rcall\0"
                                  "malloc\0"
                                  "free\0"
                                  "load\0"
                                  "aload\0"
                                  "store\0"
                                  "vload\0"
                                  "vstore\0"
                                  "udiv\0"
                                  "sdiv\0"
                                  "urem\0"
                                  "srem\0"
                                  "mul\0"
                                  "shl\0"
                                  "lshr\0"
                                  "ashr\0"
                                  "and\0"
                                  "or\0"
                                  "xor\0"
                                  "add\0"
                                  "eadd\0"
                                  "oadd\0"
                                  "sub\0"
                                  "esub\0"
                                  "osub\0"
                                  "incr\0"
                                  "decr\0"
                                  "icmp\0"
                                  "select\0"
                                  "vudiv\0"
                                  "vsdiv\0"
                                  "vurem\0"
                                  "vsrem\0"
                                  "vmul\0"
                                  "vshl\0"
                                  "vlshr\0"
                                  "vashr\0"
                                  "vand\0"
                                  "vor\0"
                                  "vxor\0"
                                  "vadd\0"
                                  "vsub\0"
                                  "vincr\0"
                                  "vdecr\0"
                                  "vicmp\0"
                                  "vselect\0"
                                  "vpudiv\0"
                                  "vpsdiv\0"
                                  "vpurem\0"
                                  "vpsrem\0"
                                  "vpmul\0"
                                  "vpand\0"
                                  "vpor\0"
                                  "vpxor\0"
                                  "vpadd\0"
                                  "vpsub\0"
                                  "vpicmp\0"
                                  "vpselect\0"
                                  "vbcast\0"
                                  "vextct\0"
                                  "vupdate\0"
                                  "const\0"};

// offset of each instruction name index. Unused opcodes default to the empty
// string at offset 0.
const std::array<unsigned, SWPP::INSTRUCTION_LIST_END> SWPPInstrNameIndices =
    [] {
      std::array<unsigned, SWPP::INSTRUCTION_LIST_END> Indices{};
      unsigned Offset = 1;
      auto AddName = [&](unsigned Opcode, const char *Name) {
        Indices[Opcode] = Offset;
        while (*Name != '\0') {
          ++Offset;
          ++Name;
        }
        ++Offset;
      };

      AddName(SWPP::PHI, "phi");
      AddName(SWPP::IMPLICIT_DEF, "implicit_def");
      AddName(SWPP::COPY, "copy");
      AddName(SWPP::RET, "ret");
      AddName(SWPP::BR, "br");
      AddName(SWPP::SWITCH, "switch");
      AddName(SWPP::CALL, "call");
      AddName(SWPP::RCALL, "rcall");
      AddName(SWPP::MALLOC, "malloc");
      AddName(SWPP::FREE, "free");
      AddName(SWPP::LOAD, "load");
      AddName(SWPP::ALOAD, "aload");
      AddName(SWPP::STORE, "store");
      AddName(SWPP::VLOAD, "vload");
      AddName(SWPP::VSTORE, "vstore");
      AddName(SWPP::UDIV, "udiv");
      AddName(SWPP::SDIV, "sdiv");
      AddName(SWPP::UREM, "urem");
      AddName(SWPP::SREM, "srem");
      AddName(SWPP::MUL, "mul");
      AddName(SWPP::SHL, "shl");
      AddName(SWPP::LSHR, "lshr");
      AddName(SWPP::ASHR, "ashr");
      AddName(SWPP::AND, "and");
      AddName(SWPP::OR, "or");
      AddName(SWPP::XOR, "xor");
      AddName(SWPP::ADD, "add");
      AddName(SWPP::EADD, "eadd");
      AddName(SWPP::OADD, "oadd");
      AddName(SWPP::SUB, "sub");
      AddName(SWPP::ESUB, "esub");
      AddName(SWPP::OSUB, "osub");
      AddName(SWPP::INCR, "incr");
      AddName(SWPP::DECR, "decr");
      AddName(SWPP::ICMP, "icmp");
      AddName(SWPP::SELECT, "select");
      AddName(SWPP::VUDIV, "vudiv");
      AddName(SWPP::VSDIV, "vsdiv");
      AddName(SWPP::VUREM, "vurem");
      AddName(SWPP::VSREM, "vsrem");
      AddName(SWPP::VMUL, "vmul");
      AddName(SWPP::VSHL, "vshl");
      AddName(SWPP::VLSHR, "vlshr");
      AddName(SWPP::VASHR, "vashr");
      AddName(SWPP::VAND, "vand");
      AddName(SWPP::VOR, "vor");
      AddName(SWPP::VXOR, "vxor");
      AddName(SWPP::VADD, "vadd");
      AddName(SWPP::VSUB, "vsub");
      AddName(SWPP::VINCR, "vincr");
      AddName(SWPP::VDECR, "vdecr");
      AddName(SWPP::VICMP, "vicmp");
      AddName(SWPP::VSELECT, "vselect");
      AddName(SWPP::VPUDIV, "vpudiv");
      AddName(SWPP::VPSDIV, "vpsdiv");
      AddName(SWPP::VPUREM, "vpurem");
      AddName(SWPP::VPSREM, "vpsrem");
      AddName(SWPP::VPMUL, "vpmul");
      AddName(SWPP::VPAND, "vpand");
      AddName(SWPP::VPOR, "vpor");
      AddName(SWPP::VPXOR, "vpxor");
      AddName(SWPP::VPADD, "vpadd");
      AddName(SWPP::VPSUB, "vpsub");
      AddName(SWPP::VPICMP, "vpicmp");
      AddName(SWPP::VPSELECT, "vpselect");
      AddName(SWPP::VBCAST, "vbcast");
      AddName(SWPP::VEXTCT, "vextct");
      AddName(SWPP::VUPDATE, "vupdate");
      AddName(SWPP::CONST, "const");
      return Indices;
    }();

static_assert(SWPPInstrNameIndices.size() == SWPP::INSTRUCTION_LIST_END,
              "Instruction Index Num Incorrect");

static inline void InitSWPPMCInstrInfo(MCInstrInfo *II) {
  II->InitMCInstrInfo(SWPPDescs.Insts, SWPPInstrNameIndices.data(),
                      SWPPInstrNameData, nullptr, nullptr,
                      SWPP::INSTRUCTION_LIST_END);
}

class SWPPInstrInfo : public TargetInstrInfo {
  const SWPPRegisterInfo RI;

public:
  // CallFrameSetupOpcode, CallFrameDestoryOpcode, CatchRetOpcode not exists in
  // SWPPTargetMachine
  SWPPInstrInfo(unsigned CFSetupOpcode = ~0u, unsigned CFDestroyOpcode = ~0u,
                unsigned CatchRetOpcode = ~0u,
                unsigned ReturnOpcode = SWPP::RET);

  const SWPPRegisterInfo &getRegisterInfo() const { return RI; }

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI, Register SrcReg,
                           bool isKill, int FrameIndex,
                           const TargetRegisterClass *RC, Register VReg,
                           MachineInstr::MIFlag Flags) const override;

  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI, Register DestReg,
                            int FrameIndex, const TargetRegisterClass *RC,
                            Register VReg, unsigned SubReg,
                            MachineInstr::MIFlag Flags) const override;
};

} // namespace llvm

#endif
