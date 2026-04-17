//===-- SWPPRegisterInfo.td - SWPP Register defs -----------*- tablegen -*-===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the SWPP register files
//===----------------------------------------------------------------------===//

class SWPPReg<string n> : Register<n> {
  let Namespace = "SWPP";
}

// Argument Registers
class Ai<bits<16> Enc, string n> : SWPPReg<n> {
  let HWEncoding = Enc;
}

// Integer registers
class Ri<bits<16> Enc, string n> : SWPPReg<n> {
  let HWEncoding = Enc;
}

// Vector Registers
class Vi<bits<16> Enc, string n> : SWPPReg<n> {
  let HWEncoding = Enc;
}

foreach l = 1-16 in {
  def arg#l : Ai<l, "arg"#l>;
  def v#l : Vi<l, "v"#l>;
}

foreach l = 1-32 in {
  def r#l : Ri<l, "r"#l>;
}

// Stack pointer
def sp : SWPPReg<"sp">;

// General Register Class (arg1 ~ arg16, r1 ~ r32)
// We freeze arg1 ~ arg16, therefore we ignore these register in allocation
// It can help coalescing argument registers
def Gen : RegisterClass<"SWPP", [i64], 8, (add
  (sequence "arg%u", 1, 16), 
  (sequence "r%u", 1, 32)
)>;

def Vec : RegisterClass<"SWPP", [v8i32, v4i64], 64, (add
  (sequence "v%u", 1, 16)
)>;
