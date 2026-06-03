//===-- AVR32InstrInfo.td - AVR32 Instruction Definitions -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "AVR32InstrFormats.td"

def SImm6AsmOperand : AsmOperandClass {
  let Name = "SImm6";
  let RenderMethod = "addImmOperands";
}

def SImm8AsmOperand : AsmOperandClass {
  let Name = "SImm8";
  let RenderMethod = "addImmOperands";
}

def SImm21AsmOperand : AsmOperandClass {
  let Name = "SImm21";
  let RenderMethod = "addImmOperands";
}

def simm6 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= -32 && Imm < 32; }]> {
  let ParserMatchClass = SImm6AsmOperand;
}

def simm8 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= -128 && Imm < 128; }]> {
  let ParserMatchClass = SImm8AsmOperand;
}

def simm21 : Operand<i32>,
             ImmLeaf<i32, [{ return Imm >= -1048576 && Imm < 1048576; }]> {
  let ParserMatchClass = SImm21AsmOperand;
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

class CompactRegRegWithPrefix<bits<3> Prefix, bits<5> Op, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Rs),
                Mnemonic#"\t$Rd, $Rs"> {
  bits<4> Rd;
  bits<4> Rs;

  let Inst{15-13} = Prefix;
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

class CompactRegImmNoOut<bits<6> Op, string Mnemonic>
    : AVR32Inst<0, (outs), (ins GPR:$Rd, simm6:$Imm),
                Mnemonic#"\t$Rd, $Imm"> {
  bits<4> Rd;
  bits<6> Imm;

  let Inst{15-10} = Op;
  let Inst{9-4} = Imm;
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

class FullRegRegReg<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rx, GPR:$Ry),
                  Mnemonic#"\t$Rd, $Rx, $Ry"> {
  bits<4> Rd;
  bits<4> Rx;
  bits<4> Ry;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rx;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Ry;
  let Inst{15-4} = Op;
  let Inst{3-0} = Rd;
}

class FullRegRegImm<bits<8> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rs, simm8:$Imm),
                  Mnemonic#"\t$Rd, $Rs, $Imm"> {
  bits<4> Rd;
  bits<4> Rs;
  bits<8> Imm;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rs;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Rd;
  let Inst{15-8} = Op;
  let Inst{7-0} = Imm;
}

class FullRegImmNoOut<bits<4> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rd, simm21:$Imm),
                  Mnemonic#"\t$Rd, $Imm"> {
  bits<4> Rd;
  bits<21> Imm;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Imm{20-17};
  let Inst{24-21} = Op;
  let Inst{20} = Imm{16};
  let Inst{19-16} = Rd;
  let Inst{15-0} = Imm{15-0};
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

def ADCrrr : FullRegRegReg<0b000000000100, "adc">;
def ABSr : CompactReg<0b010111000100, "abs">;
def ACRr : CompactReg<0b010111000000, "acr">;
def ADDrr : CompactRegReg<0b00000, "add">;
def ANDNrr : CompactRegReg<0b01000, "andn">;
def ANDrr : CompactRegReg<0b00110, "and">;
def ASRrrr : FullRegRegReg<0b000010000100, "asr">;
def BREVr : CompactReg<0b010111001001, "brev">;
def CASTS_Br : CompactReg<0b010111000110, "casts.b">;
def CASTS_Hr : CompactReg<0b010111001000, "casts.h">;
def CASTU_Br : CompactReg<0b010111000101, "castu.b">;
def CASTU_Hr : CompactReg<0b010111000111, "castu.h">;
def COMr : CompactReg<0b010111001101, "com">;
def CPCr : CompactRegNoOut<0b010111000010, "cpc">;
def CPCrr : FullRegRegNoOut<0b0001001100000000, "cpc">;
def CP_Wri21 : FullRegImmNoOut<0b0010, "cp.w">;
def CP_Wri6 : CompactRegImmNoOut<0b010110, "cp.w">;
def CP_Wrr : CompactRegRegNoOut<0b00011, "cp.w">;
def EORrr : CompactRegReg<0b00101, "eor">;
def FRS : AVR32Inst<0b1101011101000011, (outs), (ins), "frs">;
def LSLrrr : FullRegRegReg<0b000010010100, "lsl">;
def LSRrrr : FullRegRegReg<0b000010100100, "lsr">;
def MAXrrr : FullRegRegReg<0b000011000100, "max">;
def MINrrr : FullRegRegReg<0b000011010100, "min">;
def MOVrr : MovRR<0b1111, "mov">;
def MOVri8 : MovRI<0b0011, "mov">;
def MULrr : CompactRegRegWithPrefix<0b101, 0b10011, "mul">;
def MULri8 : FullRegRegImm<0b00010000, "mul">;
def MULrrr : FullRegRegReg<0b000000100100, "mul">;
def NEGr : CompactReg<0b010111000011, "neg">;
def ORrr : CompactRegReg<0b00100, "or">;
def ROLr : CompactReg<0b010111001111, "rol">;
def RORr : CompactReg<0b010111010000, "ror">;
def RSUBrr : CompactRegReg<0b00010, "rsub">;
def SBCrrr : FullRegRegReg<0b000000010100, "sbc">;
def SCRr : CompactReg<0b010111000001, "scr">;
def SUBrr : CompactRegReg<0b00001, "sub">;
def SWAP_BHr : CompactReg<0b010111001100, "swap.bh">;
def SWAP_Br : CompactReg<0b010111001011, "swap.b">;
def SWAP_Hr : CompactReg<0b010111001010, "swap.h">;
def TSTrr : CompactRegRegNoOut<0b00111, "tst">;
