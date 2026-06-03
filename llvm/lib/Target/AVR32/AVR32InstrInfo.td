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

def SImm16AsmOperand : AsmOperandClass {
  let Name = "SImm16";
  let RenderMethod = "addImmOperands";
}

def SImm21AsmOperand : AsmOperandClass {
  let Name = "SImm21";
  let RenderMethod = "addImmOperands";
}

def SubSPImmAsmOperand : AsmOperandClass {
  let Name = "SubSPImm";
  let RenderMethod = "addImmOperands";
}

def SImm12Shift1AsmOperand : AsmOperandClass {
  let Name = "SImm12Shift1";
  let RenderMethod = "addImmOperands";
}

def SImm12Shift2AsmOperand : AsmOperandClass {
  let Name = "SImm12Shift2";
  let RenderMethod = "addImmOperands";
}

def HalfPartAsmOperand : AsmOperandClass {
  let Name = "HalfPart";
  let RenderMethod = "addImmOperands";
}

def UImm5AsmOperand : AsmOperandClass {
  let Name = "UImm5";
  let RenderMethod = "addImmOperands";
}

def UImm2AsmOperand : AsmOperandClass {
  let Name = "UImm2";
  let RenderMethod = "addImmOperands";
}

def UImm3AsmOperand : AsmOperandClass {
  let Name = "UImm3";
  let RenderMethod = "addImmOperands";
}

def UImm3Shift1AsmOperand : AsmOperandClass {
  let Name = "UImm3Shift1";
  let RenderMethod = "addImmOperands";
}

def UImm4Shift2AsmOperand : AsmOperandClass {
  let Name = "UImm4Shift2";
  let RenderMethod = "addImmOperands";
}

def UImm7Shift2AsmOperand : AsmOperandClass {
  let Name = "UImm7Shift2";
  let RenderMethod = "addImmOperands";
}

def UImm8Shift2AsmOperand : AsmOperandClass {
  let Name = "UImm8Shift2";
  let RenderMethod = "addImmOperands";
}

def UImm9Shift1AsmOperand : AsmOperandClass {
  let Name = "UImm9Shift1";
  let RenderMethod = "addImmOperands";
}

def UImm9Shift2AsmOperand : AsmOperandClass {
  let Name = "UImm9Shift2";
  let RenderMethod = "addImmOperands";
}

def UImm8AsmOperand : AsmOperandClass {
  let Name = "UImm8";
  let RenderMethod = "addImmOperands";
}

def UImm9AsmOperand : AsmOperandClass {
  let Name = "UImm9";
  let RenderMethod = "addImmOperands";
}

def UImm16AsmOperand : AsmOperandClass {
  let Name = "UImm16";
  let RenderMethod = "addImmOperands";
}

def ACallDispAsmOperand : AsmOperandClass {
  let Name = "ACallDisp";
  let RenderMethod = "addImmOperands";
}

def SysRegAddrAsmOperand : AsmOperandClass {
  let Name = "SysRegAddr";
  let RenderMethod = "addImmOperands";
}

def DebugRegAddrAsmOperand : AsmOperandClass {
  let Name = "DebugRegAddr";
  let RenderMethod = "addImmOperands";
}

def IncJOSPImmAsmOperand : AsmOperandClass {
  let Name = "IncJOSPImm";
  let RenderMethod = "addImmOperands";
}

def simm6 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= -32 && Imm < 32; }]> {
  let ParserMatchClass = SImm6AsmOperand;
}

def simm8 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= -128 && Imm < 128; }]> {
  let ParserMatchClass = SImm8AsmOperand;
}

def simm16 : Operand<i32>,
             ImmLeaf<i32, [{ return Imm >= -32768 && Imm < 32768; }]> {
  let ParserMatchClass = SImm16AsmOperand;
}

def simm21 : Operand<i32>,
             ImmLeaf<i32, [{ return Imm >= -1048576 && Imm < 1048576; }]> {
  let ParserMatchClass = SImm21AsmOperand;
}

def simm12shift1 : Operand<i32>,
                   ImmLeaf<i32, [{ return Imm >= -4096 && Imm <= 4094 &&
                                          Imm % 2 == 0; }]> {
  let ParserMatchClass = SImm12Shift1AsmOperand;
  let EncoderMethod = "getShifted1OpValue";
}

def simm12shift2 : Operand<i32>,
                   ImmLeaf<i32, [{ return Imm >= -8192 && Imm <= 8188 &&
                                          Imm % 4 == 0; }]> {
  let ParserMatchClass = SImm12Shift2AsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def subspimm : Operand<i32>,
               ImmLeaf<i32, [{ return Imm >= -512 && Imm <= 508 &&
                                      Imm % 4 == 0; }]> {
  let ParserMatchClass = SubSPImmAsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def uimm2 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm < 4; }]> {
  let ParserMatchClass = UImm2AsmOperand;
}

def uimm3 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm < 8; }]> {
  let ParserMatchClass = UImm3AsmOperand;
}

def uimm3shift1 : Operand<i32>,
                  ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 14 &&
                                         Imm % 2 == 0; }]> {
  let ParserMatchClass = UImm3Shift1AsmOperand;
  let EncoderMethod = "getShifted1OpValue";
}

def uimm4shift2 : Operand<i32>,
                  ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 60 &&
                                         Imm % 4 == 0; }]> {
  let ParserMatchClass = UImm4Shift2AsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def uimm7shift2 : Operand<i32>,
                  ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 508 &&
                                         Imm % 4 == 0; }]> {
  let ParserMatchClass = UImm7Shift2AsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def uimm5 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm < 32; }]> {
  let ParserMatchClass = UImm5AsmOperand;
}

def uimm8 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm < 256; }]> {
  let ParserMatchClass = UImm8AsmOperand;
}

def uimm8shift2 : Operand<i32>,
                  ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 1020 &&
                                         Imm % 4 == 0; }]> {
  let ParserMatchClass = UImm8Shift2AsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def uimm9 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm < 512; }]> {
  let ParserMatchClass = UImm9AsmOperand;
}

def uimm9shift1 : Operand<i32>,
                  ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 1022 &&
                                         Imm % 2 == 0; }]> {
  let ParserMatchClass = UImm9Shift1AsmOperand;
  let EncoderMethod = "getShifted1OpValue";
}

def uimm9shift2 : Operand<i32>,
                  ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 2044 &&
                                         Imm % 4 == 0; }]> {
  let ParserMatchClass = UImm9Shift2AsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def uimm16 : Operand<i32>,
             ImmLeaf<i32, [{ return Imm >= 0 && Imm < 65536; }]> {
  let ParserMatchClass = UImm16AsmOperand;
}

def acalldisp : Operand<i32>,
                ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 1020 &&
                                       Imm % 4 == 0; }]> {
  let ParserMatchClass = ACallDispAsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def sysregaddr : Operand<i32>,
                 ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 1020 &&
                                        Imm % 4 == 0; }]> {
  let ParserMatchClass = SysRegAddrAsmOperand;
  let EncoderMethod = "getSysRegAddrOpValue";
}

def debugregaddr : Operand<i32>,
                   ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 1020 &&
                                          Imm % 4 == 0; }]> {
  let ParserMatchClass = DebugRegAddrAsmOperand;
  let EncoderMethod = "getSysRegAddrOpValue";
}

def incjospimm : Operand<i32>,
                 ImmLeaf<i32, [{ return Imm >= -4 && Imm <= 4 && Imm != 0; }]> {
  let ParserMatchClass = IncJOSPImmAsmOperand;
  let EncoderMethod = "getIncJOSPImmOpValue";
}

def halfpart : Operand<i32>, ImmLeaf<i32, [{ return Imm == 0 || Imm == 1; }]> {
  let ParserMatchClass = HalfPartAsmOperand;
  let PrintMethod = "printHalfPart";
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

class CompactStorePostInc<bits<5> Op, string Mnemonic>
    : AVR32Inst<0, (outs), (ins GPR:$Rp, GPR:$Rs),
                Mnemonic#"\t${Rp}++, $Rs"> {
  bits<4> Rp;
  bits<4> Rs;

  let Inst{15-13} = 0b000;
  let Inst{12-9} = Rp;
  let Inst{8-4} = Op;
  let Inst{3-0} = Rs;
}

class CompactStorePreDec<bits<5> Op, string Mnemonic>
    : AVR32Inst<0, (outs), (ins GPR:$Rp, GPR:$Rs),
                Mnemonic#"\t--$Rp, $Rs"> {
  bits<4> Rp;
  bits<4> Rs;

  let Inst{15-13} = 0b000;
  let Inst{12-9} = Rp;
  let Inst{8-4} = Op;
  let Inst{3-0} = Rs;
}

class CompactStoreDisp3<string Mnemonic>
    : AVR32Inst<0, (outs), (ins GPR:$Rp, uimm3:$Disp, GPR:$Rs),
                Mnemonic#"\t${Rp}[$Disp], $Rs"> {
  bits<4> Rp;
  bits<3> Disp;
  bits<4> Rs;

  let Inst{15-13} = 0b101;
  let Inst{12-9} = Rp;
  let Inst{8-7} = 0b01;
  let Inst{6-4} = Disp;
  let Inst{3-0} = Rs;
}

class CompactStoreDisp3Shift1<string Mnemonic>
    : AVR32Inst<0, (outs), (ins GPR:$Rp, uimm3shift1:$Disp, GPR:$Rs),
                Mnemonic#"\t${Rp}[$Disp], $Rs"> {
  bits<4> Rp;
  bits<3> Disp;
  bits<4> Rs;

  let Inst{15-13} = 0b101;
  let Inst{12-9} = Rp;
  let Inst{8-7} = 0b00;
  let Inst{6-4} = Disp;
  let Inst{3-0} = Rs;
}

class CompactStoreDisp4Shift2<string Mnemonic>
    : AVR32Inst<0, (outs), (ins GPR:$Rp, uimm4shift2:$Disp, GPR:$Rs),
                Mnemonic#"\t${Rp}[$Disp], $Rs"> {
  bits<4> Rp;
  bits<4> Disp;
  bits<4> Rs;

  let Inst{15-13} = 0b100;
  let Inst{12-9} = Rp;
  let Inst{8} = 1;
  let Inst{7-4} = Disp;
  let Inst{3-0} = Rs;
}

class FullStoreDisp16<bits<5> Op, string Mnemonic, RegisterClass SrcRC = GPR>
    : AVR32Inst32<(outs), (ins GPR:$Rp, simm16:$Disp, SrcRC:$Rs),
                  Mnemonic#"\t${Rp}[$Disp], $Rs"> {
  bits<4> Rp;
  bits<16> Disp;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = Op;
  let Inst{19-16} = Rs;
  let Inst{15-0} = Disp;
}

class FullStoreIndexShift<bits<10> Op, string Mnemonic, RegisterClass SrcRC = GPR>
    : AVR32Inst32<(outs), (ins GPR:$Rb, GPR:$Ri, uimm2:$Shift, SrcRC:$Rs),
                  Mnemonic#"\t${Rb}[$Ri << $Shift], $Rs"> {
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rb;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Ri;
  let Inst{15-6} = Op;
  let Inst{5-4} = Shift;
  let Inst{3-0} = Rs;
}

class FullStoreHalfwordPairDisp8
    : AVR32Inst32<(outs), (ins GPR:$Rp, uimm8shift2:$Disp, GPR:$Rx,
                               halfpart:$XPart, GPR:$Ry, halfpart:$YPart),
                  "sthh.w\t${Rp}[$Disp], ${Rx}:$XPart, ${Ry}:$YPart"> {
  bits<4> Rp;
  bits<8> Disp;
  bits<4> Rx;
  bits<1> XPart;
  bits<4> Ry;
  bits<1> YPart;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rx;
  let Inst{24-20} = 0b11110;
  let Inst{19-16} = Ry;
  let Inst{15-14} = 0b11;
  let Inst{13} = XPart;
  let Inst{12} = YPart;
  let Inst{11-4} = Disp;
  let Inst{3-0} = Rp;
}

class FullStoreHalfwordPairIndexShift
    : AVR32Inst32<(outs), (ins GPR:$Rb, GPR:$Ri, uimm2:$Shift, GPR:$Rx,
                               halfpart:$XPart, GPR:$Ry, halfpart:$YPart),
                  "sthh.w\t${Rb}[$Ri << $Shift], ${Rx}:$XPart, ${Ry}:$YPart"> {
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;
  bits<4> Rx;
  bits<1> XPart;
  bits<4> Ry;
  bits<1> YPart;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rx;
  let Inst{24-20} = 0b11110;
  let Inst{19-16} = Ry;
  let Inst{15-14} = 0b10;
  let Inst{13} = XPart;
  let Inst{12} = YPart;
  let Inst{11-8} = Ri;
  let Inst{7-6} = 0b00;
  let Inst{5-4} = Shift;
  let Inst{3-0} = Rb;
}

class FullStoreSwapDisp12<bits<4> Prefix, string Mnemonic, Operand DispOp>
    : AVR32Inst32<(outs), (ins GPR:$Rp, DispOp:$Disp, GPR:$Rs),
                  Mnemonic#"\t${Rp}[$Disp], $Rs"> {
  bits<4> Rp;
  bits<12> Disp;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = 0b11101;
  let Inst{19-16} = Rs;
  let Inst{15-12} = Prefix;
  let Inst{11-0} = Disp;
}

class FullStorePairDisp16<bits<5> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, simm16:$Disp, GPREven:$Rs),
                  Mnemonic#"\t${Rp}[$Disp], $Rs"> {
  bits<4> Rp;
  bits<16> Disp;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = Op;
  let Inst{19-17} = Rs{3-1};
  let Inst{16} = 1;
  let Inst{15-0} = Disp;
}

class FullStoreCondDisp9<bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, uimm9:$Disp, GPR:$Rs),
                  Mnemonic#"\t${Rp}[$Disp], $Rs"> {
  bits<4> Rp;
  bits<9> Disp;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = 0b11111;
  let Inst{19-16} = Rs;
  let Inst{15-12} = Cond;
  let Inst{11-9} = 0b111;
  let Inst{8-0} = Disp;
}

class FullStoreCondDisp9Shift1<bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, uimm9shift1:$Disp, GPR:$Rs),
                  Mnemonic#"\t${Rp}[$Disp], $Rs"> {
  bits<4> Rp;
  bits<9> Disp;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = 0b11111;
  let Inst{19-16} = Rs;
  let Inst{15-12} = Cond;
  let Inst{11-9} = 0b110;
  let Inst{8-0} = Disp;
}

class FullStoreCondDisp9Shift2<bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, uimm9shift2:$Disp, GPR:$Rs),
                  Mnemonic#"\t${Rp}[$Disp], $Rs"> {
  bits<4> Rp;
  bits<9> Disp;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = 0b11111;
  let Inst{19-16} = Rs;
  let Inst{15-12} = Cond;
  let Inst{11-9} = 0b101;
  let Inst{8-0} = Disp;
}

class CompactStorePair<bits<5> Op, bits<1> LastBit, string AsmStr>
    : AVR32Inst<0, (outs), (ins GPR:$Rp, GPREven:$Rs), AsmStr> {
  bits<4> Rp;
  bits<4> Rs;

  let Inst{15-13} = 0b101;
  let Inst{12-9} = Rp;
  let Inst{8-4} = Op;
  let Inst{3-1} = Rs{3-1};
  let Inst{0} = LastBit;
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

class CompactRegSImm8<bits<4> Prefix, string Mnemonic>
    : AVR32Inst<0, (outs GPRNoSP:$Rd), (ins simm8:$Imm),
                Mnemonic#"\t$Rd, $Imm"> {
  bits<4> Rd;
  bits<8> Imm;

  let Inst{15-12} = Prefix;
  let Inst{11-4} = Imm;
  let Inst{3-0} = Rd;
}

class CompactSPSubImm<string Mnemonic>
    : AVR32Inst<0, (outs SPOnly:$Rd), (ins subspimm:$Imm),
                Mnemonic#"\t$Rd, $Imm"> {
  bits<4> Rd;
  bits<8> Imm;

  let Inst{15-12} = 0b0010;
  let Inst{11-4} = Imm;
  let Inst{3-0} = Rd;
}

class CompactSPStoreDisp7Shift2
    : AVR32Inst<0, (outs), (ins SPOnly:$Rp, uimm7shift2:$Disp, GPR:$Rs),
                "stdsp\t${Rp}[$Disp], $Rs"> {
  bits<7> Disp;
  bits<4> Rs;

  let Inst{15-11} = 0b01010;
  let Inst{10-4} = Disp;
  let Inst{3-0} = Rs;
}

class CompactRegUImm5NoOut<bits<4> Op, string Mnemonic>
    : AVR32Inst<0, (outs), (ins GPR:$Rd, uimm5:$Imm),
                Mnemonic#"\t$Rd, $Imm"> {
  bits<4> Rd;
  bits<5> Imm;

  let Inst{15-13} = 0b101;
  let Inst{12-9} = Imm{4-1};
  let Inst{8-5} = Op;
  let Inst{4} = Imm{0};
  let Inst{3-0} = Rd;
}

class CompactImm<bits<7> Op, string Mnemonic>
    : AVR32Inst<0, (outs), (ins uimm5:$Imm), Mnemonic#"\t$Imm"> {
  bits<5> Imm;

  let Inst{15-9} = Op;
  let Inst{8-4} = Imm;
  let Inst{3-0} = 0b0011;
}

def ACALLi : AVR32Inst<0, (outs), (ins acalldisp:$Disp), "acall\t$Disp"> {
  bits<8> Disp;

  let Inst{15-12} = 0b1101;
  let Inst{11-4} = Disp;
  let Inst{3-0} = 0;
}

def INCJOSPi : AVR32Inst<0, (outs), (ins incjospimm:$Imm),
                         "incjosp\t$Imm"> {
  bits<3> Imm;

  let Inst{15-7} = 0b110101101;
  let Inst{6-4} = Imm;
  let Inst{3-0} = 0b0011;
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

class FullRegReg<bits<16> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rs),
                  Mnemonic#"\t$Rd, $Rs"> {
  bits<4> Rd;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rs;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Rd;
  let Inst{15-0} = Op;
}

class FullRegUImm5NoOut<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rd, uimm5:$Imm),
                  Mnemonic#"\t$Rd, $Imm"> {
  bits<4> Rd;
  bits<5> Imm;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rd;
  let Inst{15-5} = 0;
  let Inst{4-0} = Imm;
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

class FullRegRegRegShift<bits<10> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rx, GPR:$Ry, uimm2:$Shift),
                  Mnemonic#"\t$Rd, $Rx, $Ry << $Shift"> {
  bits<4> Rd;
  bits<4> Rx;
  bits<4> Ry;
  bits<2> Shift;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rx;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Ry;
  let Inst{15-6} = Op;
  let Inst{5-4} = Shift;
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

class FullRegRegSImm16<bits<5> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rs, simm16:$Imm),
                  Mnemonic#"\t$Rd, $Rs, $Imm"> {
  bits<4> Rd;
  bits<4> Rs;
  bits<16> Imm;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rs;
  let Inst{24-20} = Op;
  let Inst{19-16} = Rd;
  let Inst{15-0} = Imm;
}

class FullRegShiftBits<bits<12> Prefix, bits<6> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins uimm5:$Sa, uimm5:$Bp),
                  Mnemonic#"\t$Rd >> $Sa, $Bp"> {
  bits<4> Rd;
  bits<5> Sa;
  bits<5> Bp;

  let Inst{31-20} = Prefix;
  let Inst{19-16} = Rd;
  let Inst{15-10} = Op;
  let Inst{9-5} = Bp;
  let Inst{4-0} = Sa;
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

class FullRegImm<bits<4> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins simm21:$Imm),
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

class FullImm8NoOut<bits<16> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins uimm8:$Imm), Mnemonic#"\t$Imm"> {
  bits<8> Imm;

  let Inst{31-16} = Op;
  let Inst{15-8} = 0;
  let Inst{7-0} = Imm;
}

class FullRegUImm16<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins uimm16:$Imm),
                  Mnemonic#"\t$Rd, $Imm"> {
  bits<4> Rd;
  bits<16> Imm;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rd;
  let Inst{15-0} = Imm;
}

class FullRegUImm16Suffix<bits<12> Op, string Mnemonic, string Suffix>
    : AVR32Inst32<(outs GPR:$Rd), (ins uimm16:$Imm),
                  Mnemonic#"\t$Rd, $Imm"#Suffix> {
  bits<4> Rd;
  bits<16> Imm;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rd;
  let Inst{15-0} = Imm;
}

class FullRegSysReg<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins sysregaddr:$Addr),
                  Mnemonic#"\t$Rd, $Addr"> {
  bits<4> Rd;
  bits<8> Addr;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rd;
  let Inst{15-8} = 0;
  let Inst{7-0} = Addr;
}

class FullSysRegReg<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins sysregaddr:$Addr, GPR:$Rs),
                  Mnemonic#"\t$Addr, $Rs"> {
  bits<8> Addr;
  bits<4> Rs;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rs;
  let Inst{15-8} = 0;
  let Inst{7-0} = Addr;
}

class FullRegDebugReg<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins debugregaddr:$Addr),
                  Mnemonic#"\t$Rd, $Addr"> {
  bits<4> Rd;
  bits<8> Addr;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rd;
  let Inst{15-8} = 0;
  let Inst{7-0} = Addr;
}

class FullDebugRegReg<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins debugregaddr:$Addr, GPR:$Rs),
                  Mnemonic#"\t$Addr, $Rs"> {
  bits<8> Addr;
  bits<4> Rs;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rs;
  let Inst{15-8} = 0;
  let Inst{7-0} = Addr;
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

class CondRegImm8<bits<12> Prefix, bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins simm8:$Imm),
                  Mnemonic#"\t$Rd, $Imm"> {
  bits<4> Rd;
  bits<8> Imm;

  let Inst{31-20} = Prefix;
  let Inst{19-16} = Rd;
  let Inst{15-12} = 0;
  let Inst{11-8} = Cond;
  let Inst{7-0} = Imm;
}

class MovCondRI<bits<4> Cond, string Mnemonic>
    : CondRegImm8<0b111110011011, Cond, Mnemonic>;

class SubCondRI<bits<4> Cond, string Mnemonic>
    : CondRegImm8<0b111101111011, Cond, Mnemonic>;

class SubfCondRI<bits<4> Cond, string Mnemonic>
    : CondRegImm8<0b111101011011, Cond, Mnemonic>;

class CondRRR<bits<4> Cond, bits<4> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rx, GPR:$Ry),
                  Mnemonic#"\t$Rd, $Rx, $Ry"> {
  bits<4> Rd;
  bits<4> Rx;
  bits<4> Ry;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rx;
  let Inst{24-20} = 0b11101;
  let Inst{19-16} = Ry;
  let Inst{15-12} = 0b1110;
  let Inst{11-8} = Cond;
  let Inst{7-4} = Op;
  let Inst{3-0} = Rd;
}

class RetCond<bits<4> Cond, string Mnemonic>
    : AVR32Inst<0, (outs), (ins GPR:$Rs), Mnemonic#"\t$Rs"> {
  bits<4> Rs;

  let hasCtrlDep = 1;
  let Inst{15-8} = 0b01011110;
  let Inst{7-4} = Cond;
  let Inst{3-0} = Rs;
  let isBranch = 1;
  let isReturn = 1;
  let isTerminator = 1;
}

class SrCond<bits<4> Cond, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins), Mnemonic#"\t$Rd"> {
  bits<4> Rd;

  let Inst{15-8} = 0b01011111;
  let Inst{7-4} = Cond;
  let Inst{3-0} = Rd;
}

def ADCrrr : FullRegRegReg<0b000000000100, "adc">;
def ABSr : CompactReg<0b010111000100, "abs">;
def ACRr : CompactReg<0b010111000000, "acr">;
def ADDABSrrr : FullRegRegReg<0b000011100100, "addabs">;
def ADDALrrr : CondRRR<0b1111, 0b0000, "addal">;
def ADDEQrrr : CondRRR<0b0000, 0b0000, "addeq">;
def ADDNErrr : CondRRR<0b0001, 0b0000, "addne">;
def ADDCCrrr : CondRRR<0b0010, 0b0000, "addcc">;
def ADDHSrrr : CondRRR<0b0010, 0b0000, "addhs">;
def ADDCSrrr : CondRRR<0b0011, 0b0000, "addcs">;
def ADDLOrrr : CondRRR<0b0011, 0b0000, "addlo">;
def ADDGErrr : CondRRR<0b0100, 0b0000, "addge">;
def ADDLTrrr : CondRRR<0b0101, 0b0000, "addlt">;
def ADDMIrrr : CondRRR<0b0110, 0b0000, "addmi">;
def ADDPLrrr : CondRRR<0b0111, 0b0000, "addpl">;
def ADDLSrrr : CondRRR<0b1000, 0b0000, "addls">;
def ADDGTrrr : CondRRR<0b1001, 0b0000, "addgt">;
def ADDLErrr : CondRRR<0b1010, 0b0000, "addle">;
def ADDHIrrr : CondRRR<0b1011, 0b0000, "addhi">;
def ADDVSrrr : CondRRR<0b1100, 0b0000, "addvs">;
def ADDVCrrr : CondRRR<0b1101, 0b0000, "addvc">;
def ADDQSrrr : CondRRR<0b1110, 0b0000, "addqs">;
def ADDrr : CompactRegReg<0b00000, "add">;
def ANDHri : FullRegUImm16<0b111001000001, "andh">;
def ANDHri_COH : FullRegUImm16Suffix<0b111001100001, "andh", ", coh">;
def ANDLri : FullRegUImm16<0b111000000001, "andl">;
def ANDLri_COH : FullRegUImm16Suffix<0b111000100001, "andl", ", coh">;
def ANDALrrr : CondRRR<0b1111, 0b0010, "andal">;
def ANDEQrrr : CondRRR<0b0000, 0b0010, "andeq">;
def ANDNErrr : CondRRR<0b0001, 0b0010, "andne">;
def ANDCCrrr : CondRRR<0b0010, 0b0010, "andcc">;
def ANDHSrrr : CondRRR<0b0010, 0b0010, "andhs">;
def ANDCSrrr : CondRRR<0b0011, 0b0010, "andcs">;
def ANDLOrrr : CondRRR<0b0011, 0b0010, "andlo">;
def ANDGErrr : CondRRR<0b0100, 0b0010, "andge">;
def ANDLTrrr : CondRRR<0b0101, 0b0010, "andlt">;
def ANDMIrrr : CondRRR<0b0110, 0b0010, "andmi">;
def ANDPLrrr : CondRRR<0b0111, 0b0010, "andpl">;
def ANDLSrrr : CondRRR<0b1000, 0b0010, "andls">;
def ANDGTrrr : CondRRR<0b1001, 0b0010, "andgt">;
def ANDLErrr : CondRRR<0b1010, 0b0010, "andle">;
def ANDHIrrr : CondRRR<0b1011, 0b0010, "andhi">;
def ANDVSrrr : CondRRR<0b1100, 0b0010, "andvs">;
def ANDVCrrr : CondRRR<0b1101, 0b0010, "andvc">;
def ANDQSrrr : CondRRR<0b1110, 0b0010, "andqs">;
def ANDNrr : CompactRegReg<0b01000, "andn">;
def ANDrr : CompactRegReg<0b00110, "and">;
def ASRrrr : FullRegRegReg<0b000010000100, "asr">;
def BLDri : FullRegUImm5NoOut<0b111011011011, "bld">;
def BREVr : CompactReg<0b010111001001, "brev">;
def BSTri : FullRegUImm5NoOut<0b111011111011, "bst">;
def BREAKPOINT : AVR32Inst<0b1101011001110011, (outs), (ins), "breakpoint">;
def CBRri : CompactRegUImm5NoOut<0b1110, "cbr">;
def CASTS_Br : CompactReg<0b010111000110, "casts.b">;
def CASTS_Hr : CompactReg<0b010111001000, "casts.h">;
def CASTU_Br : CompactReg<0b010111000101, "castu.b">;
def CASTU_Hr : CompactReg<0b010111000111, "castu.h">;
def CLZrr : FullRegReg<0b0001001000000000, "clz">;
def COMr : CompactReg<0b010111001101, "com">;
def CPCr : CompactRegNoOut<0b010111000010, "cpc">;
def CPCrr : FullRegRegNoOut<0b0001001100000000, "cpc">;
def CP_Brr : FullRegRegNoOut<0b0001100000000000, "cp.b">;
def CP_Hrr : FullRegRegNoOut<0b0001100100000000, "cp.h">;
def CP_Wri21 : FullRegImmNoOut<0b0010, "cp.w">;
def CP_Wri6 : CompactRegImmNoOut<0b010110, "cp.w">;
def CP_Wrr : CompactRegRegNoOut<0b00011, "cp.w">;
def CSRFCZi : CompactImm<0b1101000, "csrfcz">;
def CSRFi : CompactImm<0b1101010, "csrf">;
def DIVSrrr : FullRegRegReg<0b000011000000, "divs">;
def DIVUrrr : FullRegRegReg<0b000011010000, "divu">;
def EORHri : FullRegUImm16<0b111011100001, "eorh">;
def EORLri : FullRegUImm16<0b111011000001, "eorl">;
def EORALrrr : CondRRR<0b1111, 0b0100, "eoral">;
def EOREQrrr : CondRRR<0b0000, 0b0100, "eoreq">;
def EORNErrr : CondRRR<0b0001, 0b0100, "eorne">;
def EORCCrrr : CondRRR<0b0010, 0b0100, "eorcc">;
def EORHSrrr : CondRRR<0b0010, 0b0100, "eorhs">;
def EORCSrrr : CondRRR<0b0011, 0b0100, "eorcs">;
def EORLOrrr : CondRRR<0b0011, 0b0100, "eorlo">;
def EORGErrr : CondRRR<0b0100, 0b0100, "eorge">;
def EORLTrrr : CondRRR<0b0101, 0b0100, "eorlt">;
def EORMIrrr : CondRRR<0b0110, 0b0100, "eormi">;
def EORPLrrr : CondRRR<0b0111, 0b0100, "eorpl">;
def EORLSrrr : CondRRR<0b1000, 0b0100, "eorls">;
def EORGTrrr : CondRRR<0b1001, 0b0100, "eorgt">;
def EORLErrr : CondRRR<0b1010, 0b0100, "eorle">;
def EORHIrrr : CondRRR<0b1011, 0b0100, "eorhi">;
def EORVSrrr : CondRRR<0b1100, 0b0100, "eorvs">;
def EORVCrrr : CondRRR<0b1101, 0b0100, "eorvc">;
def EORQSrrr : CondRRR<0b1110, 0b0100, "eorqs">;
def EORrr : CompactRegReg<0b00101, "eor">;
def FRS : AVR32Inst<0b1101011101000011, (outs), (ins), "frs">;
def ICALLr : CompactRegNoOut<0b010111010001, "icall">;
def LSLrrr : FullRegRegReg<0b000010010100, "lsl">;
def LSRrrr : FullRegRegReg<0b000010100100, "lsr">;
def MAXrrr : FullRegRegReg<0b000011000100, "max">;
def MINrrr : FullRegRegReg<0b000011010100, "min">;
def MFDRi : FullRegDebugReg<0b111001011011, "mfdr">;
def MFSRi : FullRegSysReg<0b111000011011, "mfsr">;
def MOVALri : MovCondRI<0b1111, "moval">;
def MOVEQri : MovCondRI<0b0000, "moveq">;
def MOVNEri : MovCondRI<0b0001, "movne">;
def MOVCCri : MovCondRI<0b0010, "movcc">;
def MOVHSri : MovCondRI<0b0010, "movhs">;
def MOVCSri : MovCondRI<0b0011, "movcs">;
def MOVLOri : MovCondRI<0b0011, "movlo">;
def MOVGEri : MovCondRI<0b0100, "movge">;
def MOVLTri : MovCondRI<0b0101, "movlt">;
def MOVMIri : MovCondRI<0b0110, "movmi">;
def MOVPLri : MovCondRI<0b0111, "movpl">;
def MOVLSri : MovCondRI<0b1000, "movls">;
def MOVGTri : MovCondRI<0b1001, "movgt">;
def MOVLEri : MovCondRI<0b1010, "movle">;
def MOVHIri : MovCondRI<0b1011, "movhi">;
def MOVVSri : MovCondRI<0b1100, "movvs">;
def MOVVCri : MovCondRI<0b1101, "movvc">;
def MOVQSri : MovCondRI<0b1110, "movqs">;
def MOVALrr : MovRR<0b1111, "moval">;
def MOVEQrr : MovRR<0b0000, "moveq">;
def MOVNErr : MovRR<0b0001, "movne">;
def MOVCCrr : MovRR<0b0010, "movcc">;
def MOVHSrr : MovRR<0b0010, "movhs">;
def MOVCSrr : MovRR<0b0011, "movcs">;
def MOVLOrr : MovRR<0b0011, "movlo">;
def MOVGErr : MovRR<0b0100, "movge">;
def MOVLTrr : MovRR<0b0101, "movlt">;
def MOVMIrr : MovRR<0b0110, "movmi">;
def MOVPLrr : MovRR<0b0111, "movpl">;
def MOVLSrr : MovRR<0b1000, "movls">;
def MOVGTrr : MovRR<0b1001, "movgt">;
def MOVLErr : MovRR<0b1010, "movle">;
def MOVHIrr : MovRR<0b1011, "movhi">;
def MOVVSrr : MovRR<0b1100, "movvs">;
def MOVVCrr : MovRR<0b1101, "movvc">;
def MOVQSrr : MovRR<0b1110, "movqs">;
def MOVrr : MovRR<0b1111, "mov">;
def MOVri8 : MovRI<0b0011, "mov">;
def MOVri21 : FullRegImm<0b0011, "mov">;
def MOVHri : FullRegUImm16<0b111111000001, "movh">;
def MULrr : CompactRegRegWithPrefix<0b101, 0b10011, "mul">;
def MULri8 : FullRegRegImm<0b00010000, "mul">;
def MULrrr : FullRegRegReg<0b000000100100, "mul">;
def MULS_Drrr : FullRegRegReg<0b000001000100, "muls.d">;
def MULU_Drrr : FullRegRegReg<0b000001100100, "mulu.d">;
def MUSFRr : CompactRegNoOut<0b010111010011, "musfr">;
def MUSTRr : CompactReg<0b010111010010, "mustr">;
def MTDRi : FullDebugRegReg<0b111001111011, "mtdr">;
def MTSRi : FullSysRegReg<0b111000111011, "mtsr">;
def NEGr : CompactReg<0b010111000011, "neg">;
def ORHri : FullRegUImm16<0b111010100001, "orh">;
def ORLri : FullRegUImm16<0b111010000001, "orl">;
def ORALrrr : CondRRR<0b1111, 0b0011, "oral">;
def OREQrrr : CondRRR<0b0000, 0b0011, "oreq">;
def ORNErrr : CondRRR<0b0001, 0b0011, "orne">;
def ORCCrrr : CondRRR<0b0010, 0b0011, "orcc">;
def ORHSrrr : CondRRR<0b0010, 0b0011, "orhs">;
def ORCSrrr : CondRRR<0b0011, 0b0011, "orcs">;
def ORLOrrr : CondRRR<0b0011, 0b0011, "orlo">;
def ORGErrr : CondRRR<0b0100, 0b0011, "orge">;
def ORLTrrr : CondRRR<0b0101, 0b0011, "orlt">;
def ORMIrrr : CondRRR<0b0110, 0b0011, "ormi">;
def ORPLrrr : CondRRR<0b0111, 0b0011, "orpl">;
def ORLSrrr : CondRRR<0b1000, 0b0011, "orls">;
def ORGTrrr : CondRRR<0b1001, 0b0011, "orgt">;
def ORLErrr : CondRRR<0b1010, 0b0011, "orle">;
def ORHIrrr : CondRRR<0b1011, 0b0011, "orhi">;
def ORVSrrr : CondRRR<0b1100, 0b0011, "orvs">;
def ORVCrrr : CondRRR<0b1101, 0b0011, "orvc">;
def ORQSrrr : CondRRR<0b1110, 0b0011, "orqs">;
def ORrr : CompactRegReg<0b00100, "or">;
def POPJC : AVR32Inst<0b1101011100010011, (outs), (ins), "popjc">;
def PUSHJC : AVR32Inst<0b1101011100100011, (outs), (ins), "pushjc">;
let isBarrier = 1, isReturn = 1, isTerminator = 1 in {
def RETD : AVR32Inst<0b1101011000100011, (outs), (ins), "retd">;
def RETE : AVR32Inst<0b1101011000000011, (outs), (ins), "rete">;
}
def RETALr : RetCond<0b1111, "retal">;
def RETEQr : RetCond<0b0000, "reteq">;
def RETNEr : RetCond<0b0001, "retne">;
def RETCCr : RetCond<0b0010, "retcc">;
def RETHSr : RetCond<0b0010, "reths">;
def RETCSr : RetCond<0b0011, "retcs">;
def RETLOr : RetCond<0b0011, "retlo">;
def RETGEr : RetCond<0b0100, "retge">;
def RETLTr : RetCond<0b0101, "retlt">;
def RETMIr : RetCond<0b0110, "retmi">;
def RETPLr : RetCond<0b0111, "retpl">;
def RETLSr : RetCond<0b1000, "retls">;
def RETGTr : RetCond<0b1001, "retgt">;
def RETLEr : RetCond<0b1010, "retle">;
def RETHIr : RetCond<0b1011, "rethi">;
def RETVSr : RetCond<0b1100, "retvs">;
def RETVCr : RetCond<0b1101, "retvc">;
def RETQSr : RetCond<0b1110, "retqs">;
def : InstAlias<"ret", (RETALr LR), 0>;
let isBarrier = 1, isReturn = 1, isTerminator = 1 in {
def RETJ : AVR32Inst<0b1101011000110011, (outs), (ins), "retj">;
def RETS : AVR32Inst<0b1101011000010011, (outs), (ins), "rets">;
def RETSS : AVR32Inst<0b1101011101100011, (outs), (ins), "retss">;
}
def ROLr : CompactReg<0b010111001111, "rol">;
def RORr : CompactReg<0b010111010000, "ror">;
def RSUBALri : CondRegImm8<0b111110111011, 0b1111, "rsubal">;
def RSUBEQri : CondRegImm8<0b111110111011, 0b0000, "rsubeq">;
def RSUBNEri : CondRegImm8<0b111110111011, 0b0001, "rsubne">;
def RSUBCCri : CondRegImm8<0b111110111011, 0b0010, "rsubcc">;
def RSUBHSri : CondRegImm8<0b111110111011, 0b0010, "rsubhs">;
def RSUBCSri : CondRegImm8<0b111110111011, 0b0011, "rsubcs">;
def RSUBLOri : CondRegImm8<0b111110111011, 0b0011, "rsublo">;
def RSUBGEri : CondRegImm8<0b111110111011, 0b0100, "rsubge">;
def RSUBLTri : CondRegImm8<0b111110111011, 0b0101, "rsublt">;
def RSUBMIri : CondRegImm8<0b111110111011, 0b0110, "rsubmi">;
def RSUBPLri : CondRegImm8<0b111110111011, 0b0111, "rsubpl">;
def RSUBLSri : CondRegImm8<0b111110111011, 0b1000, "rsubls">;
def RSUBGTri : CondRegImm8<0b111110111011, 0b1001, "rsubgt">;
def RSUBLEri : CondRegImm8<0b111110111011, 0b1010, "rsuble">;
def RSUBHIri : CondRegImm8<0b111110111011, 0b1011, "rsubhi">;
def RSUBVSri : CondRegImm8<0b111110111011, 0b1100, "rsubvs">;
def RSUBVCri : CondRegImm8<0b111110111011, 0b1101, "rsubvc">;
def RSUBQSri : CondRegImm8<0b111110111011, 0b1110, "rsubqs">;
def RSUBrr : CompactRegReg<0b00010, "rsub">;
def SATADD_Wrrr : FullRegRegReg<0b000000001100, "satadd.w">;
def SATRNDSri : FullRegShiftBits<0b111100111011, 0b000000, "satrnds">;
def SATRNDUri : FullRegShiftBits<0b111100111011, 0b000001, "satrndu">;
def SATSUB_Hrrr : FullRegRegReg<0b000000111100, "satsub.h">;
def SATSUB_Wrrr : FullRegRegReg<0b000000011100, "satsub.w">;
def SATSUB_Wrri16 : FullRegRegSImm16<0b01101, "satsub.w">;
def SATSri : FullRegShiftBits<0b111100011011, 0b000000, "sats">;
def SATUri : FullRegShiftBits<0b111100011011, 0b000001, "satu">;
def SBCrrr : FullRegRegReg<0b000000010100, "sbc">;
def SBRri : CompactRegUImm5NoOut<0b1101, "sbr">;
def SCALL : AVR32Inst<0b1101011100110011, (outs), (ins), "scall">;
def SSCALL : AVR32Inst<0b1101011101010011, (outs), (ins), "sscall">;
def SCRr : CompactReg<0b010111000001, "scr">;
def SLEEPi : FullImm8NoOut<0b1110001101100000, "sleep">;
def SRALr : SrCond<0b1111, "sral">;
def SREQr : SrCond<0b0000, "sreq">;
def SRNEr : SrCond<0b0001, "srne">;
def SRCCr : SrCond<0b0010, "srcc">;
def SRHSr : SrCond<0b0010, "srhs">;
def SRCSr : SrCond<0b0011, "srcs">;
def SRLOr : SrCond<0b0011, "srlo">;
def SRGEr : SrCond<0b0100, "srge">;
def SRLTr : SrCond<0b0101, "srlt">;
def SRMIr : SrCond<0b0110, "srmi">;
def SRPLr : SrCond<0b0111, "srpl">;
def SRLSr : SrCond<0b1000, "srls">;
def SRGTr : SrCond<0b1001, "srgt">;
def SRLEr : SrCond<0b1010, "srle">;
def SRHIr : SrCond<0b1011, "srhi">;
def SRVSr : SrCond<0b1100, "srvs">;
def SRVCr : SrCond<0b1101, "srvc">;
def SRQSr : SrCond<0b1110, "srqs">;
def SSRFi : CompactImm<0b1101001, "ssrf">;
def ST_B_PostInc : CompactStorePostInc<0b01100, "st.b">;
def ST_B_PreDec : CompactStorePreDec<0b01111, "st.b">;
def ST_B_Disp3 : CompactStoreDisp3<"st.b">;
def ST_B_Disp16 : FullStoreDisp16<0b10110, "st.b">;
def ST_B_IndexShift : FullStoreIndexShift<0b0000101100, "st.b">;
def ST_B_AL_Disp9 : FullStoreCondDisp9<0b1111, "st.bal">;
def ST_B_EQ_Disp9 : FullStoreCondDisp9<0b0000, "st.beq">;
def ST_B_NE_Disp9 : FullStoreCondDisp9<0b0001, "st.bne">;
def ST_B_CC_Disp9 : FullStoreCondDisp9<0b0010, "st.bcc">;
def ST_B_HS_Disp9 : FullStoreCondDisp9<0b0010, "st.bhs">;
def ST_B_CS_Disp9 : FullStoreCondDisp9<0b0011, "st.bcs">;
def ST_B_LO_Disp9 : FullStoreCondDisp9<0b0011, "st.blo">;
def ST_B_GE_Disp9 : FullStoreCondDisp9<0b0100, "st.bge">;
def ST_B_LT_Disp9 : FullStoreCondDisp9<0b0101, "st.blt">;
def ST_B_MI_Disp9 : FullStoreCondDisp9<0b0110, "st.bmi">;
def ST_B_PL_Disp9 : FullStoreCondDisp9<0b0111, "st.bpl">;
def ST_B_LS_Disp9 : FullStoreCondDisp9<0b1000, "st.bls">;
def ST_B_GT_Disp9 : FullStoreCondDisp9<0b1001, "st.bgt">;
def ST_B_LE_Disp9 : FullStoreCondDisp9<0b1010, "st.ble">;
def ST_B_HI_Disp9 : FullStoreCondDisp9<0b1011, "st.bhi">;
def ST_B_VS_Disp9 : FullStoreCondDisp9<0b1100, "st.bvs">;
def ST_B_VC_Disp9 : FullStoreCondDisp9<0b1101, "st.bvc">;
def ST_B_QS_Disp9 : FullStoreCondDisp9<0b1110, "st.bqs">;
def ST_D_PostInc : CompactStorePair<0b10010, 0, "st.d\t${Rp}++, $Rs">;
def ST_D_PreDec : CompactStorePair<0b10010, 1, "st.d\t--$Rp, $Rs">;
def ST_D_Reg : CompactStorePair<0b10001, 1, "st.d\t$Rp, $Rs">;
def ST_D_Disp16 : FullStorePairDisp16<0b01110, "st.d">;
def ST_D_IndexShift : FullStoreIndexShift<0b0000100000, "st.d", GPREven>;
def ST_H_PostInc : CompactStorePostInc<0b01011, "st.h">;
def ST_H_PreDec : CompactStorePreDec<0b01110, "st.h">;
def ST_H_Disp3 : CompactStoreDisp3Shift1<"st.h">;
def ST_H_Disp16 : FullStoreDisp16<0b10101, "st.h">;
def ST_H_IndexShift : FullStoreIndexShift<0b0000101000, "st.h">;
def ST_H_AL_Disp9 : FullStoreCondDisp9Shift1<0b1111, "st.hal">;
def ST_H_EQ_Disp9 : FullStoreCondDisp9Shift1<0b0000, "st.heq">;
def ST_H_NE_Disp9 : FullStoreCondDisp9Shift1<0b0001, "st.hne">;
def ST_H_CC_Disp9 : FullStoreCondDisp9Shift1<0b0010, "st.hcc">;
def ST_H_HS_Disp9 : FullStoreCondDisp9Shift1<0b0010, "st.hhs">;
def ST_H_CS_Disp9 : FullStoreCondDisp9Shift1<0b0011, "st.hcs">;
def ST_H_LO_Disp9 : FullStoreCondDisp9Shift1<0b0011, "st.hlo">;
def ST_H_GE_Disp9 : FullStoreCondDisp9Shift1<0b0100, "st.hge">;
def ST_H_LT_Disp9 : FullStoreCondDisp9Shift1<0b0101, "st.hlt">;
def ST_H_MI_Disp9 : FullStoreCondDisp9Shift1<0b0110, "st.hmi">;
def ST_H_PL_Disp9 : FullStoreCondDisp9Shift1<0b0111, "st.hpl">;
def ST_H_LS_Disp9 : FullStoreCondDisp9Shift1<0b1000, "st.hls">;
def ST_H_GT_Disp9 : FullStoreCondDisp9Shift1<0b1001, "st.hgt">;
def ST_H_LE_Disp9 : FullStoreCondDisp9Shift1<0b1010, "st.hle">;
def ST_H_HI_Disp9 : FullStoreCondDisp9Shift1<0b1011, "st.hhi">;
def ST_H_VS_Disp9 : FullStoreCondDisp9Shift1<0b1100, "st.hvs">;
def ST_H_VC_Disp9 : FullStoreCondDisp9Shift1<0b1101, "st.hvc">;
def ST_H_QS_Disp9 : FullStoreCondDisp9Shift1<0b1110, "st.hqs">;
def ST_W_PostInc : CompactStorePostInc<0b01010, "st.w">;
def ST_W_PreDec : CompactStorePreDec<0b01101, "st.w">;
def ST_W_Disp4 : CompactStoreDisp4Shift2<"st.w">;
def ST_W_Disp16 : FullStoreDisp16<0b10100, "st.w">;
def ST_W_IndexShift : FullStoreIndexShift<0b0000100100, "st.w">;
def ST_W_AL_Disp9 : FullStoreCondDisp9Shift2<0b1111, "st.wal">;
def ST_W_EQ_Disp9 : FullStoreCondDisp9Shift2<0b0000, "st.weq">;
def ST_W_NE_Disp9 : FullStoreCondDisp9Shift2<0b0001, "st.wne">;
def ST_W_CC_Disp9 : FullStoreCondDisp9Shift2<0b0010, "st.wcc">;
def ST_W_HS_Disp9 : FullStoreCondDisp9Shift2<0b0010, "st.whs">;
def ST_W_CS_Disp9 : FullStoreCondDisp9Shift2<0b0011, "st.wcs">;
def ST_W_LO_Disp9 : FullStoreCondDisp9Shift2<0b0011, "st.wlo">;
def ST_W_GE_Disp9 : FullStoreCondDisp9Shift2<0b0100, "st.wge">;
def ST_W_LT_Disp9 : FullStoreCondDisp9Shift2<0b0101, "st.wlt">;
def ST_W_MI_Disp9 : FullStoreCondDisp9Shift2<0b0110, "st.wmi">;
def ST_W_PL_Disp9 : FullStoreCondDisp9Shift2<0b0111, "st.wpl">;
def ST_W_LS_Disp9 : FullStoreCondDisp9Shift2<0b1000, "st.wls">;
def ST_W_GT_Disp9 : FullStoreCondDisp9Shift2<0b1001, "st.wgt">;
def ST_W_LE_Disp9 : FullStoreCondDisp9Shift2<0b1010, "st.wle">;
def ST_W_HI_Disp9 : FullStoreCondDisp9Shift2<0b1011, "st.whi">;
def ST_W_VS_Disp9 : FullStoreCondDisp9Shift2<0b1100, "st.wvs">;
def ST_W_VC_Disp9 : FullStoreCondDisp9Shift2<0b1101, "st.wvc">;
def ST_W_QS_Disp9 : FullStoreCondDisp9Shift2<0b1110, "st.wqs">;
def STCOND : FullStoreDisp16<0b10111, "stcond">;
def STDSP : CompactSPStoreDisp7Shift2;
def STHH_W_Disp8 : FullStoreHalfwordPairDisp8;
def STHH_W_IndexShift : FullStoreHalfwordPairIndexShift;
def STSWP_H_Disp12 : FullStoreSwapDisp12<0b1001, "stswp.h", simm12shift1>;
def STSWP_W_Disp12 : FullStoreSwapDisp12<0b1010, "stswp.w", simm12shift2>;
def SUBALrrr : CondRRR<0b1111, 0b0001, "subal">;
def SUBEQrrr : CondRRR<0b0000, 0b0001, "subeq">;
def SUBNErrr : CondRRR<0b0001, 0b0001, "subne">;
def SUBCCrrr : CondRRR<0b0010, 0b0001, "subcc">;
def SUBHSrrr : CondRRR<0b0010, 0b0001, "subhs">;
def SUBCSrrr : CondRRR<0b0011, 0b0001, "subcs">;
def SUBLOrrr : CondRRR<0b0011, 0b0001, "sublo">;
def SUBGErrr : CondRRR<0b0100, 0b0001, "subge">;
def SUBLTrrr : CondRRR<0b0101, 0b0001, "sublt">;
def SUBMIrrr : CondRRR<0b0110, 0b0001, "submi">;
def SUBPLrrr : CondRRR<0b0111, 0b0001, "subpl">;
def SUBLSrrr : CondRRR<0b1000, 0b0001, "subls">;
def SUBGTrrr : CondRRR<0b1001, 0b0001, "subgt">;
def SUBLErrr : CondRRR<0b1010, 0b0001, "suble">;
def SUBHIrrr : CondRRR<0b1011, 0b0001, "subhi">;
def SUBVSrrr : CondRRR<0b1100, 0b0001, "subvs">;
def SUBVCrrr : CondRRR<0b1101, 0b0001, "subvc">;
def SUBQSrrr : CondRRR<0b1110, 0b0001, "subqs">;
def SUBALri : SubCondRI<0b1111, "subal">;
def SUBEQri : SubCondRI<0b0000, "subeq">;
def SUBNEri : SubCondRI<0b0001, "subne">;
def SUBCCri : SubCondRI<0b0010, "subcc">;
def SUBHSri : SubCondRI<0b0010, "subhs">;
def SUBCSri : SubCondRI<0b0011, "subcs">;
def SUBLOri : SubCondRI<0b0011, "sublo">;
def SUBGEri : SubCondRI<0b0100, "subge">;
def SUBLTri : SubCondRI<0b0101, "sublt">;
def SUBMIri : SubCondRI<0b0110, "submi">;
def SUBPLri : SubCondRI<0b0111, "subpl">;
def SUBLSri : SubCondRI<0b1000, "subls">;
def SUBGTri : SubCondRI<0b1001, "subgt">;
def SUBLEri : SubCondRI<0b1010, "suble">;
def SUBHIri : SubCondRI<0b1011, "subhi">;
def SUBVSri : SubCondRI<0b1100, "subvs">;
def SUBVCri : SubCondRI<0b1101, "subvc">;
def SUBQSri : SubCondRI<0b1110, "subqs">;
def SUBFALri : SubfCondRI<0b1111, "subfal">;
def SUBFEQri : SubfCondRI<0b0000, "subfeq">;
def SUBFNEri : SubfCondRI<0b0001, "subfne">;
def SUBFCCri : SubfCondRI<0b0010, "subfcc">;
def SUBFHSri : SubfCondRI<0b0010, "subfhs">;
def SUBFCSri : SubfCondRI<0b0011, "subfcs">;
def SUBFLOri : SubfCondRI<0b0011, "subflo">;
def SUBFGEri : SubfCondRI<0b0100, "subfge">;
def SUBFLTri : SubfCondRI<0b0101, "subflt">;
def SUBFMIri : SubfCondRI<0b0110, "subfmi">;
def SUBFPLri : SubfCondRI<0b0111, "subfpl">;
def SUBFLSri : SubfCondRI<0b1000, "subfls">;
def SUBFGTri : SubfCondRI<0b1001, "subfgt">;
def SUBFLEri : SubfCondRI<0b1010, "subfle">;
def SUBFHIri : SubfCondRI<0b1011, "subfhi">;
def SUBFVSri : SubfCondRI<0b1100, "subfvs">;
def SUBFVCri : SubfCondRI<0b1101, "subfvc">;
def SUBFQSri : SubfCondRI<0b1110, "subfqs">;
def SUBSPri8 : CompactSPSubImm<"sub">;
def SUBri8 : CompactRegSImm8<0b0010, "sub">;
def SUBrrrs : FullRegRegRegShift<0b0000000100, "sub">;
def SUBrri16 : FullRegRegSImm16<0b01100, "sub">;
def SUBri21 : FullRegImm<0b0001, "sub">;
def SUBrr : CompactRegReg<0b00001, "sub">;
def SWAP_BHr : CompactReg<0b010111001100, "swap.bh">;
def SWAP_Br : CompactReg<0b010111001011, "swap.b">;
def SWAP_Hr : CompactReg<0b010111001010, "swap.h">;
def SYNCi : FullImm8NoOut<0b1110011101100000, "sync">;
def TLBR : AVR32Inst<0b1101011001000011, (outs), (ins), "tlbr">;
def TLBS : AVR32Inst<0b1101011001010011, (outs), (ins), "tlbs">;
def TLBW : AVR32Inst<0b1101011001100011, (outs), (ins), "tlbw">;
def TNBZr : CompactRegNoOut<0b010111001110, "tnbz">;
def TSTrr : CompactRegRegNoOut<0b00111, "tst">;
