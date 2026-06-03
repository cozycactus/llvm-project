//===-- AVR32InstrInfo.td - AVR32 Instruction Definitions -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "AVR32InstrFormats.td"

def SImm8AsmOperand : AsmOperandClass {
  let Name = "SImm8";
  let RenderMethod = "addImmOperands";
}

def simm8 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= -128 && Imm < 128; }]> {
  let ParserMatchClass = SImm8AsmOperand;
}

def NOP : AVR32Inst<0b1101011100000011, (outs), (ins), "nop">;

class CompactRegReg<bits<5> Op, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Rs),
                Mnemonic#"\t$Rd, $Rs"> {
  bits<4> Rd;
  bits<4> Rs;

  let Inst{15-13} = 0b000;
  let Inst{12-9} = Rs;
  let Inst{8-4} = Op;
  let Inst{3-0} = Rd;
}

class CompactReg<bits<12> Op, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins), Mnemonic#"\t$Rd"> {
  bits<4> Rd;

  let Inst{15-4} = Op;
  let Inst{3-0} = Rd;
}

class MovRI<bits<4> Prefix, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins simm8:$Imm),
                Mnemonic#"\t$Rd, $Imm"> {
  bits<4> Rd;
  bits<8> Imm;

  let Inst{15-12} = Prefix;
  let Inst{11-4} = Imm;
  let Inst{3-0} = Rd;
}

class MovRR<bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rs), Mnemonic#"\t$Rd, $Rs"> {
  bits<4> Rd;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rs;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Rd;
  let Inst{15-12} = 0b0001;
  let Inst{11-8} = 0b0111;
  let Inst{7-4} = Cond;
  let Inst{3-0} = 0b0000;
}

def ACRr : CompactReg<0b010111000000, "acr">;
def ADDrr : CompactRegReg<0b00000, "add">;
def ANDNrr : CompactRegReg<0b01000, "andn">;
def ANDrr : CompactRegReg<0b00110, "and">;
def COMr : CompactReg<0b010111001101, "com">;
def EORrr : CompactRegReg<0b00101, "eor">;
def MOVrr : MovRR<0b1111, "mov">;
def MOVri8 : MovRI<0b0011, "mov">;
def NEGr : CompactReg<0b010111000011, "neg">;
def ORrr : CompactRegReg<0b00100, "or">;
def RSUBrr : CompactRegReg<0b00010, "rsub">;
def SUBrr : CompactRegReg<0b00001, "sub">;
