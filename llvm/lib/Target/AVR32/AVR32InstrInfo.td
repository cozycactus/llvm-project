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

class CompactRegRegNoOut<bits<5> Op, string Mnemonic>
    : AVR32Inst<0, (outs), (ins GPR:$Rd, GPR:$Rs),
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

class CompactRegNoOut<bits<12> Op, string Mnemonic>
    : AVR32Inst<0, (outs), (ins GPR:$Rd), Mnemonic#"\t$Rd"> {
  bits<4> Rd;

  let Inst{15-4} = Op;
  let Inst{3-0} = Rd;
}

class FullRegRegNoOut<bits<16> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rd, GPR:$Rs),
                  Mnemonic#"\t$Rd, $Rs"> {
  bits<4> Rd;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rs;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Rd;
  let Inst{15-0} = Op;
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

def ABSr : CompactReg<0b010111000100, "abs">;
def ACRr : CompactReg<0b010111000000, "acr">;
def ADDrr : CompactRegReg<0b00000, "add">;
def ANDNrr : CompactRegReg<0b01000, "andn">;
def ANDrr : CompactRegReg<0b00110, "and">;
def BREVr : CompactReg<0b010111001001, "brev">;
def CASTS_Br : CompactReg<0b010111000110, "casts.b">;
def CASTS_Hr : CompactReg<0b010111001000, "casts.h">;
def CASTU_Br : CompactReg<0b010111000101, "castu.b">;
def CASTU_Hr : CompactReg<0b010111000111, "castu.h">;
def COMr : CompactReg<0b010111001101, "com">;
def CPCr : CompactRegNoOut<0b010111000010, "cpc">;
def CPCrr : FullRegRegNoOut<0b0001001100000000, "cpc">;
def CP_Wrr : CompactRegRegNoOut<0b00011, "cp.w">;
def EORrr : CompactRegReg<0b00101, "eor">;
def FRS : AVR32Inst<0b1101011101000011, (outs), (ins), "frs">;
def MOVrr : MovRR<0b1111, "mov">;
def MOVri8 : MovRI<0b0011, "mov">;
def NEGr : CompactReg<0b010111000011, "neg">;
def ORrr : CompactRegReg<0b00100, "or">;
def ROLr : CompactReg<0b010111001111, "rol">;
def RORr : CompactReg<0b010111010000, "ror">;
def RSUBrr : CompactRegReg<0b00010, "rsub">;
def SCRr : CompactReg<0b010111000001, "scr">;
def SUBrr : CompactRegReg<0b00001, "sub">;
def SWAP_BHr : CompactReg<0b010111001100, "swap.bh">;
def SWAP_Br : CompactReg<0b010111001011, "swap.b">;
def SWAP_Hr : CompactReg<0b010111001010, "swap.h">;
