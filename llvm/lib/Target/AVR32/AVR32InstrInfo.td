//===-- AVR32InstrInfo.td - AVR32 Instruction Definitions -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "AVR32InstrFormats.td"

def SDT_AVR32Call : SDTypeProfile<0, -1, [SDTCisVT<0, iPTR>]>;
def SDT_AVR32Cmp : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;
def SDT_AVR32BrCC : SDTypeProfile<0, 2, [SDTCisVT<0, OtherVT>,
                                         SDTCisVT<1, i32>]>;
def SDT_AVR32SetCC : SDTypeProfile<1, 3, [SDTCisVT<0, i32>,
                                          SDTCisSameAs<0, 1>,
                                          SDTCisSameAs<1, 2>,
                                          SDTCisVT<3, i32>]>;
def SDT_AVR32SetCC64 : SDTypeProfile<1, 5, [SDTCisVT<0, i32>,
                                            SDTCisVT<1, i32>,
                                            SDTCisVT<2, i32>,
                                            SDTCisVT<3, i32>,
                                            SDTCisVT<4, i32>,
                                            SDTCisVT<5, i32>]>;
def SDT_AVR32SelectCC : SDTypeProfile<1, 5, [SDTCisVT<0, i32>,
                                             SDTCisSameAs<1, 2>,
                                             SDTCisSameAs<0, 3>,
                                             SDTCisSameAs<0, 4>,
                                             SDTCisVT<5, i32>]>;
def SDT_AVR32DivRem : SDTypeProfile<1, 2, [SDTCisVT<0, untyped>,
                                           SDTCisVT<1, i32>,
                                           SDTCisVT<2, i32>]>;
def SDT_AVR32Div : SDTypeProfile<1, 2, [SDTCisVT<0, i32>,
                                        SDTCisSameAs<0, 1>,
                                        SDTCisSameAs<0, 2>]>;

def AVR32call : SDNode<"AVR32ISD::CALL", SDT_AVR32Call,
                       [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue,
                        SDNPVariadic]>;
def AVR32callseq_start :
    SDNode<"ISD::CALLSEQ_START",
           SDCallSeqStart<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>,
           [SDNPHasChain, SDNPOutGlue]>;
def AVR32callseq_end :
    SDNode<"ISD::CALLSEQ_END",
           SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>,
           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def AVR32cmp : SDNode<"AVR32ISD::CMP", SDT_AVR32Cmp, [SDNPOutGlue]>;
def AVR32cmpc : SDNode<"AVR32ISD::CMP_CARRY", SDT_AVR32Cmp,
                       [SDNPInGlue, SDNPOutGlue]>;
def AVR32brcc : SDNode<"AVR32ISD::BR_CC", SDT_AVR32BrCC,
                       [SDNPHasChain, SDNPInGlue]>;
def AVR32setcc : SDNode<"AVR32ISD::SET_CC", SDT_AVR32SetCC>;
def AVR32setcc64 : SDNode<"AVR32ISD::SET_CC_64", SDT_AVR32SetCC64>;
def AVR32selectcc : SDNode<"AVR32ISD::SELECT_CC", SDT_AVR32SelectCC>;
def AVR32retflag : SDNode<"AVR32ISD::RET_FLAG", SDTNone,
                          [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def AVR32retintrflag : SDNode<"AVR32ISD::RET_INTR_FLAG", SDTNone,
                              [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def AVR32sdivrem : SDNode<"AVR32ISD::SDIVREM", SDT_AVR32DivRem>;
def AVR32udivrem : SDNode<"AVR32ISD::UDIVREM", SDT_AVR32DivRem>;
def AVR32sdiv : SDNode<"AVR32ISD::SDIV", SDT_AVR32Div>;
def AVR32udiv : SDNode<"AVR32ISD::UDIV", SDT_AVR32Div>;
def AVR32umullohi : SDNode<"AVR32ISD::UMUL_LOHI", SDT_AVR32DivRem,
                           [SDNPCommutative]>;

def AVR32_COND_EQ : PatLeaf<(i32 0)>;
def AVR32_COND_NE : PatLeaf<(i32 1)>;
def AVR32_COND_CC : PatLeaf<(i32 2)>;
def AVR32_COND_CS : PatLeaf<(i32 3)>;
def AVR32_COND_GE : PatLeaf<(i32 4)>;
def AVR32_COND_LT : PatLeaf<(i32 5)>;
def AVR32_COND_LS : PatLeaf<(i32 8)>;
def AVR32_COND_GT : PatLeaf<(i32 9)>;
def AVR32_COND_LE : PatLeaf<(i32 10)>;
def AVR32_COND_HI : PatLeaf<(i32 11)>;

def SImm6AsmOperand : AsmOperandClass {
  let Name = "SImm6";
  let RenderMethod = "addImmOperands";
}

def SImm8AsmOperand : AsmOperandClass {
  let Name = "SImm8";
  let RenderMethod = "addImmOperands";
}

def SImm8Shift1AsmOperand : AsmOperandClass {
  let Name = "SImm8Shift1";
  let RenderMethod = "addImmOperands";
}

def SImm10Shift1AsmOperand : AsmOperandClass {
  let Name = "SImm10Shift1";
  let RenderMethod = "addImmOperands";
}

def SImm10PCRelShift1AsmOperand : AsmOperandClass {
  let Name = "SImm10PCRelShift1";
  let RenderMethod = "addImmOperands";
}

def SImm11AsmOperand : AsmOperandClass {
  let Name = "SImm11";
  let RenderMethod = "addImmOperands";
}

def SImm12AsmOperand : AsmOperandClass {
  let Name = "SImm12";
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

def SImm21Shift1AsmOperand : AsmOperandClass {
  let Name = "SImm21Shift1";
  let RenderMethod = "addImmOperands";
}

def LdaExprAsmOperand : AsmOperandClass {
  let Name = "LdaExpr";
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

def SImm15Shift2AsmOperand : AsmOperandClass {
  let Name = "SImm15Shift2";
  let RenderMethod = "addImmOperands";
}

def SImm16Shift2AsmOperand : AsmOperandClass {
  let Name = "SImm16Shift2";
  let RenderMethod = "addImmOperands";
}

def SImm16PCRelShift2AsmOperand : AsmOperandClass {
  let Name = "SImm16PCRelShift2";
  let RenderMethod = "addImmOperands";
}

def SImm16PCRelAsmOperand : AsmOperandClass {
  let Name = "SImm16PCRel";
  let RenderMethod = "addImmOperands";
}

def HalfPartAsmOperand : AsmOperandClass {
  let Name = "HalfPart";
  let RenderMethod = "addImmOperands";
}

def BytePartAsmOperand : AsmOperandClass {
  let Name = "BytePart";
  let RenderMethod = "addImmOperands";
}

def FixedShift2AsmOperand : AsmOperandClass {
  let Name = "FixedShift2";
  let RenderMethod = "addImmOperands";
}

def UImm5AsmOperand : AsmOperandClass {
  let Name = "UImm5";
  let RenderMethod = "addImmOperands";
}

def UImm7AsmOperand : AsmOperandClass {
  let Name = "UImm7";
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

def UImm4AsmOperand : AsmOperandClass {
  let Name = "UImm4";
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

def UImm5Shift2AsmOperand : AsmOperandClass {
  let Name = "UImm5Shift2";
  let RenderMethod = "addImmOperands";
}

def UImm7Shift2AsmOperand : AsmOperandClass {
  let Name = "UImm7Shift2";
  let RenderMethod = "addImmOperands";
}

def UImm7PCRelShift2AsmOperand : AsmOperandClass {
  let Name = "UImm7PCRelShift2";
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

def UImm12Shift2AsmOperand : AsmOperandClass {
  let Name = "UImm12Shift2";
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

def RegList16AsmOperand : AsmOperandClass {
  let Name = "RegList16";
  let RenderMethod = "addImmOperands";
}

def RegList8AsmOperand : AsmOperandClass {
  let Name = "RegList8";
  let RenderMethod = "addImmOperands";
}

def PopmRetValAsmOperand : AsmOperandClass {
  let Name = "PopmRetVal";
  let RenderMethod = "addImmOperands";
}

def CoprocessorAsmOperand : AsmOperandClass {
  let Name = "Coprocessor";
  let RenderMethod = "addImmOperands";
}

def CoprocessorRegisterAsmOperand : AsmOperandClass {
  let Name = "CoprocessorRegister";
  let RenderMethod = "addImmOperands";
}

def CoprocessorEvenRegisterAsmOperand : AsmOperandClass {
  let Name = "CoprocessorEvenRegister";
  let RenderMethod = "addImmOperands";
}

def CoprocessorRegListDAsmOperand : AsmOperandClass {
  let Name = "CoprocessorRegListD";
  let RenderMethod = "addImmOperands";
}

def CoprocessorRegListLowAsmOperand : AsmOperandClass {
  let Name = "CoprocessorRegListLow";
  let RenderMethod = "addImmOperands";
}

def CoprocessorRegListHighAsmOperand : AsmOperandClass {
  let Name = "CoprocessorRegListHigh";
  let RenderMethod = "addImmOperands";
}

def PicoInAsmOperand : AsmOperandClass {
  let Name = "PicoIn";
  let RenderMethod = "addImmOperands";
}

def PicoRegisterAsmOperand : AsmOperandClass {
  let Name = "PicoRegister";
  let RenderMethod = "addImmOperands";
}

def PicoEvenRegisterAsmOperand : AsmOperandClass {
  let Name = "PicoEvenRegister";
  let RenderMethod = "addImmOperands";
}

def PicoRegListDAsmOperand : AsmOperandClass {
  let Name = "PicoRegListD";
  let RenderMethod = "addImmOperands";
}

def PicoRegListLowAsmOperand : AsmOperandClass {
  let Name = "PicoRegListLow";
  let RenderMethod = "addImmOperands";
}

def PicoRegListHighAsmOperand : AsmOperandClass {
  let Name = "PicoRegListHigh";
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

def simm8shift1 : Operand<i32>,
                  ImmLeaf<i32, [{ return Imm >= -256 && Imm <= 254 &&
                                         Imm % 2 == 0; }]> {
  let ParserMatchClass = SImm8Shift1AsmOperand;
  let EncoderMethod = "getShifted1OpValue";
}

def simm10shift1 : Operand<i32>,
                   ImmLeaf<i32, [{ return Imm >= -1024 && Imm <= 1022 &&
                                          Imm % 2 == 0; }]> {
  let ParserMatchClass = SImm10Shift1AsmOperand;
  let EncoderMethod = "getShifted1OpValue";
}

def simm10pcrelshift1 : Operand<i32>,
                        ImmLeaf<i32, [{ return Imm >= -1024 &&
                                               Imm <= 1022 &&
                                               Imm % 2 == 0; }]> {
  let ParserMatchClass = SImm10PCRelShift1AsmOperand;
  let EncoderMethod = "getPCRel11HOpValue";
}

def simm11 : Operand<i32>,
             ImmLeaf<i32, [{ return Imm >= -1024 && Imm < 1024; }]> {
  let ParserMatchClass = SImm11AsmOperand;
}

def simm12 : Operand<i32>,
             ImmLeaf<i32, [{ return Imm >= -2048 && Imm < 2048; }]> {
  let ParserMatchClass = SImm12AsmOperand;
}

def simm16 : Operand<i32>,
             ImmLeaf<i32, [{ return Imm >= -32768 && Imm < 32768; }]> {
  let ParserMatchClass = SImm16AsmOperand;
}

def simm21 : Operand<i32>,
             ImmLeaf<i32, [{ return Imm >= -1048576 && Imm < 1048576; }]> {
  let ParserMatchClass = SImm21AsmOperand;
  let EncoderMethod = "getSImm21OpValue";
}

def simm21shift1 : Operand<i32>,
                   ImmLeaf<i32, [{ return Imm >= -2097152 &&
                                          Imm <= 2097150 &&
                                          Imm % 2 == 0; }]> {
  let ParserMatchClass = SImm21Shift1AsmOperand;
  let EncoderMethod = "getShifted1OpValue";
}

def simm21pcrelshift1 : Operand<i32>,
                        ImmLeaf<i32, [{ return Imm >= -2097152 &&
                                               Imm <= 2097150 &&
                                               Imm % 2 == 0; }]> {
  let ParserMatchClass = SImm21Shift1AsmOperand;
  let EncoderMethod = "getPCRel22HOpValue";
}

def brtarget22 : Operand<OtherVT> {
  let EncoderMethod = "getPCRel22HOpValue";
}

def brtarget9 : Operand<OtherVT> {
  let EncoderMethod = "getPCRel9HOpValue";
}

def brtarget11 : Operand<OtherVT> {
  let EncoderMethod = "getPCRel11HOpValue";
}

def ldaexpr : Operand<i32> {
  let ParserMatchClass = LdaExprAsmOperand;
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

def simm15shift2 : Operand<i32>,
                   ImmLeaf<i32, [{ return Imm >= -65536 && Imm <= 65532 &&
                                          Imm % 4 == 0; }]> {
  let ParserMatchClass = SImm15Shift2AsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def simm16shift2 : Operand<i32>,
                   ImmLeaf<i32, [{ return Imm >= -131072 && Imm <= 131068 &&
                                          Imm % 4 == 0; }]> {
  let ParserMatchClass = SImm16Shift2AsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def simm16pcrelshift2 : Operand<i32>,
                        ImmLeaf<i32, [{ return Imm >= -131072 &&
                                               Imm <= 131068 &&
                                               Imm % 4 == 0; }]> {
  let ParserMatchClass = SImm16PCRelShift2AsmOperand;
  let EncoderMethod = "getPCRel16WOpValue";
}

def simm16pcrel : Operand<i32> {
  let ParserMatchClass = SImm16PCRelAsmOperand;
  let EncoderMethod = "getPCRel16BOpValue";
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

def uimm4 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm < 16; }]> {
  let ParserMatchClass = UImm4AsmOperand;
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

def uimm5shift2 : Operand<i32>,
                  ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 124 &&
                                         Imm % 4 == 0; }]> {
  let ParserMatchClass = UImm5Shift2AsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def uimm7shift2 : Operand<i32>,
                  ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 508 &&
                                         Imm % 4 == 0; }]> {
  let ParserMatchClass = UImm7Shift2AsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def uimm7pcrelshift2 : Operand<i32>,
                       ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 508 &&
                                              Imm % 4 == 0; }]> {
  let ParserMatchClass = UImm7PCRelShift2AsmOperand;
  let EncoderMethod = "getPCRel7WOpValue";
}

def uimm5 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm < 32; }]> {
  let ParserMatchClass = UImm5AsmOperand;
}

def uimm7 : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm < 128; }]> {
  let ParserMatchClass = UImm7AsmOperand;
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

def uimm12shift2 : Operand<i32>,
                   ImmLeaf<i32, [{ return Imm >= 0 && Imm <= 16380 &&
                                          Imm % 4 == 0; }]> {
  let ParserMatchClass = UImm12Shift2AsmOperand;
  let EncoderMethod = "getShifted2OpValue";
}

def uimm16 : Operand<i32>,
             ImmLeaf<i32, [{ return Imm >= 0 && Imm < 65536; }]> {
  let ParserMatchClass = UImm16AsmOperand;
  let EncoderMethod = "getUImm16OpValue";
}

def reglist16 : Operand<i32>,
                ImmLeaf<i32, [{ return Imm > 0 && Imm < 65536; }]> {
  let ParserMatchClass = RegList16AsmOperand;
  let PrintMethod = "printRegList16";
}

def reglist8 : Operand<i32>,
               ImmLeaf<i32, [{ return Imm > 0 && Imm < 256; }]> {
  let ParserMatchClass = RegList8AsmOperand;
  let PrintMethod = "printRegList8";
}

def popmretval : Operand<i32>,
                 ImmLeaf<i32, [{ return Imm >= -1 && Imm <= 1; }]> {
  let ParserMatchClass = PopmRetValAsmOperand;
  let PrintMethod = "printPopmRetVal";
  let EncoderMethod = "getPopmRetValOpValue";
}

def coprocessor : Operand<i32>,
                  ImmLeaf<i32, [{ return Imm >= 0 && Imm < 8; }]> {
  let ParserMatchClass = CoprocessorAsmOperand;
  let PrintMethod = "printCoprocessor";
}

def coprocessorreg : Operand<i32>,
                     ImmLeaf<i32, [{ return Imm >= 0 && Imm < 16; }]> {
  let ParserMatchClass = CoprocessorRegisterAsmOperand;
  let PrintMethod = "printCoprocessorRegister";
}

def coprocessorevenreg : Operand<i32>,
                         ImmLeaf<i32, [{ return Imm >= 0 && Imm < 16 &&
                                                Imm % 2 == 0; }]> {
  let ParserMatchClass = CoprocessorEvenRegisterAsmOperand;
  let PrintMethod = "printCoprocessorRegister";
}

def coprocessorreglistd : Operand<i32>,
                          ImmLeaf<i32, [{ return Imm > 0 && Imm < 256; }]> {
  let ParserMatchClass = CoprocessorRegListDAsmOperand;
  let PrintMethod = "printCoprocessorRegListD";
}

def coprocessorreglistlow : Operand<i32>,
                             ImmLeaf<i32, [{ return Imm > 0 &&
                                                    Imm < 256; }]> {
  let ParserMatchClass = CoprocessorRegListLowAsmOperand;
  let PrintMethod = "printCoprocessorRegListLow";
}

def coprocessorreglisthigh : Operand<i32>,
                              ImmLeaf<i32, [{ return (Imm & 0xff) == 0 &&
                                                     Imm > 0 &&
                                                     Imm < 65536; }]> {
  let ParserMatchClass = CoprocessorRegListHighAsmOperand;
  let PrintMethod = "printCoprocessorRegListHigh";
}

def picoin : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm < 12; }]> {
  let ParserMatchClass = PicoInAsmOperand;
  let PrintMethod = "printPicoIn";
}

def picoreg : Operand<i32>, ImmLeaf<i32, [{ return Imm >= 0 && Imm < 16; }]> {
  let ParserMatchClass = PicoRegisterAsmOperand;
  let PrintMethod = "printPicoRegister";
}

def picoevenreg : Operand<i32>,
                  ImmLeaf<i32, [{ return Imm >= 0 && Imm < 16 &&
                                         Imm % 2 == 0; }]> {
  let ParserMatchClass = PicoEvenRegisterAsmOperand;
  let PrintMethod = "printPicoRegister";
}

def picoreglistd : Operand<i32>, ImmLeaf<i32, [{ return Imm > 0 && Imm < 256; }]> {
  let ParserMatchClass = PicoRegListDAsmOperand;
  let PrintMethod = "printPicoRegListD";
}

def picoreglistlow : Operand<i32>, ImmLeaf<i32, [{ return Imm > 0 && Imm < 256; }]> {
  let ParserMatchClass = PicoRegListLowAsmOperand;
  let PrintMethod = "printPicoRegListLow";
}

def picoreglisthigh : Operand<i32>,
                      ImmLeaf<i32, [{ return Imm > 0 && Imm < 65536 &&
                                             (Imm & 0xff) == 0; }]> {
  let ParserMatchClass = PicoRegListHighAsmOperand;
  let PrintMethod = "printPicoRegListHigh";
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

def GPRDivPairOp : RegisterOperand<GPRDivPair, "printGPRDivPairOperand">;

def bfext_width : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(N->getAPIntValue().popcount(), SDLoc(N),
                                   MVT::i32);
}]>;

def bfext_mask : PatLeaf<(i32 imm), [{
  uint64_t Imm = N->getZExtValue();
  return Imm != 0 && Imm != UINT64_C(0xffffffff) && isMask_32(Imm);
}], bfext_width>;

def halfpart : Operand<i32>, ImmLeaf<i32, [{ return Imm == 0 || Imm == 1; }]> {
  let ParserMatchClass = HalfPartAsmOperand;
  let PrintMethod = "printHalfPart";
}

def bytepart : Operand<i32>,
               ImmLeaf<i32, [{ return Imm >= 0 && Imm < 4; }]> {
  let ParserMatchClass = BytePartAsmOperand;
  let PrintMethod = "printBytePart";
}

def fixedshift2 : Operand<i32>, ImmLeaf<i32, [{ return Imm == 2; }]> {
  let ParserMatchClass = FixedShift2AsmOperand;
}

def NOP : AVR32Inst<0b1101011100000011, (outs), (ins), "nop">;

class CompactRegReg<bits<5> Op, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Rs),
                Mnemonic#"\t$Rd, $Rs"> {
  let Defs = [SREG];

  bits<4> Rd;
  bits<4> Rs;

  let Inst{15-13} = 0b000;
  let Inst{12-9} = Rs;
  let Inst{8-4} = Op;
  let Inst{3-0} = Rd;
}

class CompactRegRegCG<bits<5> Op, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Old, GPR:$Rs),
                Mnemonic#"\t$Rd, $Rs"> {
  let Defs = [SREG];

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
  let Defs = [SREG];

  bits<4> Rd;
  bits<4> Rs;

  let Inst{15-13} = 0b000;
  let Inst{12-9} = Rs;
  let Inst{8-4} = Op;
  let Inst{3-0} = Rd;
}

class CompactStorePostInc<bits<5> Op, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rp_wb), (ins GPR:$Rp, GPR:$Rs),
                Mnemonic#"\t${Rp}++, $Rs"> {
  let mayStore = 1;
  let hasExtraDefRegAllocReq = 1;
  let Constraints = "$Rp = $Rp_wb,@earlyclobber $Rp_wb";

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
  let mayStore = 1;

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
  let mayStore = 1;

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
  let mayStore = 1;

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
  let mayStore = 1;

  bits<4> Rp;
  bits<4> Disp;
  bits<4> Rs;

  let Inst{15-13} = 0b100;
  let Inst{12-9} = Rp;
  let Inst{8} = 1;
  let Inst{7-4} = Disp;
  let Inst{3-0} = Rs;
}

class CompactLoadPostInc<bits<5> Op, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd, GPR:$Rp_wb), (ins GPR:$Rp),
                Mnemonic#"\t$Rd, ${Rp}++"> {
  let mayLoad = 1;
  let hasExtraDefRegAllocReq = 1;
  let Constraints = "$Rp = $Rp_wb,@earlyclobber $Rd";

  bits<4> Rd;
  bits<4> Rp;

  let Inst{15-13} = 0b000;
  let Inst{12-9} = Rp;
  let Inst{8-4} = Op;
  let Inst{3-0} = Rd;
}

class CompactLoadPreDec<bits<5> Op, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Rp),
                Mnemonic#"\t$Rd, --$Rp"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;

  let Inst{15-13} = 0b000;
  let Inst{12-9} = Rp;
  let Inst{8-4} = Op;
  let Inst{3-0} = Rd;
}

class CompactLoadDisp5Shift2<string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Rp, uimm5shift2:$Disp),
                Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<5> Disp;

  let Inst{15-13} = 0b011;
  let Inst{12-9} = Rp;
  let Inst{8-4} = Disp;
  let Inst{3-0} = Rd;
}

class CompactLoadDisp3<string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Rp, uimm3:$Disp),
                Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<3> Disp;

  let Inst{15-13} = 0b000;
  let Inst{12-9} = Rp;
  let Inst{8-7} = 0b11;
  let Inst{6-4} = Disp;
  let Inst{3-0} = Rd;
}

class CompactLoadDisp3Shift1<string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Rp, uimm3shift1:$Disp),
                Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<3> Disp;

  let Inst{15-13} = 0b100;
  let Inst{12-9} = Rp;
  let Inst{8-7} = 0b00;
  let Inst{6-4} = Disp;
  let Inst{3-0} = Rd;
}

class CompactLoadDisp3Shift1Sub<bits<2> Sub, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Rp, uimm3shift1:$Disp),
                Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<3> Disp;

  let Inst{15-13} = 0b100;
  let Inst{12-9} = Rp;
  let Inst{8-7} = Sub;
  let Inst{6-4} = Disp;
  let Inst{3-0} = Rd;
}

class CompactLoadBaseDisp7Shift2<bits<5> Op, RegisterClass BaseRC,
                                 string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins BaseRC:$Rp, uimm7shift2:$Disp),
                Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<7> Disp;

  let Inst{15-11} = Op;
  let Inst{10-4} = Disp;
  let Inst{3-0} = Rd;
}

class CompactLoadBasePCRel7Shift2<bits<5> Op, RegisterClass BaseRC,
                                  string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins BaseRC:$Rp, uimm7pcrelshift2:$Disp),
                Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;
  let isCodeGenOnly = 1;

  bits<4> Rd;
  bits<7> Disp;

  let Inst{15-11} = Op;
  let Inst{10-4} = Disp;
  let Inst{3-0} = Rd;
}

class FullStoreDisp16<bits<5> Op, string Mnemonic, RegisterClass SrcRC = GPR>
    : AVR32Inst32<(outs), (ins GPR:$Rp, simm16:$Disp, SrcRC:$Rs),
                  Mnemonic#"\t${Rp}[$Disp], $Rs"> {
  let mayStore = 1;

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
  let mayStore = 1;

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

class FullLoadDisp16<bits<5> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rp, simm16:$Disp),
                  Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<16> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = Op;
  let Inst{19-16} = Rd;
  let Inst{15-0} = Disp;
}

class FullLoadPCRel16<bits<5> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins PCOnly:$Rp, simm16pcrel:$Disp),
                  Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;
  let isCodeGenOnly = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<16> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = Op;
  let Inst{19-16} = Rd;
  let Inst{15-0} = Disp;
}

class FullLoadIndexShift<bits<10> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rb, GPR:$Ri, uimm2:$Shift),
                  Mnemonic#"\t$Rd, ${Rb}[$Ri << $Shift]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rb;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Ri;
  let Inst{15-6} = Op;
  let Inst{5-4} = Shift;
  let Inst{3-0} = Rd;
}

class FullLoadIndexShiftPC<bits<10> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rb, GPR:$Ri, uimm2:$Shift),
                  Mnemonic#"\tpc, ${Rb}[$Ri << $Shift]"> {
  let mayLoad = 1;
  let isBranch = 1;
  let isIndirectBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
  let isCodeGenOnly = 1;
  let Defs = [PC];

  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rb;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Ri;
  let Inst{15-6} = Op;
  let Inst{5-4} = Shift;
  let Inst{3-0} = 0b1111;
}

class FullLoadIndexPart<string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rb, GPR:$Ri, bytepart:$Part,
                                       fixedshift2:$Shift),
                  Mnemonic#"\t$Rd, ${Rb}[${Ri}:$Part << $Shift]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Part;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rb;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Ri;
  let Inst{15-6} = 0b0000111110;
  let Inst{5-4} = Part;
  let Inst{3-0} = Rd;
}

class FullLoadPairDisp16<bits<5> Op, string Mnemonic>
    : AVR32Inst32<(outs GPREven:$Rd), (ins GPR:$Rp, simm16:$Disp),
                  Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<16> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = Op;
  let Inst{19-17} = Rd{3-1};
  let Inst{16} = 0;
  let Inst{15-0} = Disp;
}

class FullMemDisp16<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, simm16:$Disp),
                  Mnemonic#"\t${Rp}[$Disp]"> {
  bits<4> Rp;
  bits<16> Disp;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-0} = Disp;
}

class FullMemDisp16Shift2<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, simm16shift2:$Disp),
                  Mnemonic#"\t${Rp}[$Disp]"> {
  bits<4> Rp;
  bits<16> Disp;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-0} = Disp;
}

class FullMemPCRel16Shift2<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins PCOnly:$Rp, simm16pcrelshift2:$Disp),
                  Mnemonic#"\t${Rp}[$Disp]"> {
  bits<4> Rp;
  bits<16> Disp;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-0} = Disp;
}

class FullMemDisp11UImm5<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, simm11:$Disp, uimm5:$Op5),
                  Mnemonic#"\t${Rp}[$Disp], $Op5"> {
  bits<4> Rp;
  bits<11> Disp;
  bits<5> Op5;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-11} = Op5;
  let Inst{10-0} = Disp;
}

class FullLoadPairIndexShift<bits<10> Op, string Mnemonic>
    : AVR32Inst32<(outs GPREven:$Rd), (ins GPR:$Rb, GPR:$Ri, uimm2:$Shift),
                  Mnemonic#"\t$Rd, ${Rb}[$Ri << $Shift]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rb;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Ri;
  let Inst{15-6} = Op;
  let Inst{5-4} = Shift;
  let Inst{3-1} = Rd{3-1};
  let Inst{0} = 0;
}

class FullLoadInsertByteDisp12
    : AVR32Inst32<(outs GPR:$Rd), (ins bytepart:$Part, GPR:$Rp, simm12:$Disp),
                  "ldins.b\t${Rd}:$Part, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<2> Part;
  bits<4> Rp;
  bits<12> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = 0b11101;
  let Inst{19-16} = Rd;
  let Inst{15-14} = 0b01;
  let Inst{13-12} = Part;
  let Inst{11-0} = Disp;
}

class FullLoadInsertHalfDisp12
    : AVR32Inst32<(outs GPR:$Rd),
                  (ins halfpart:$Part, GPR:$Rp, simm12shift1:$Disp),
                  "ldins.h\t${Rd}:$Part, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<1> Part;
  bits<4> Rp;
  bits<12> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = 0b11101;
  let Inst{19-16} = Rd;
  let Inst{15-13} = 0b000;
  let Inst{12} = Part;
  let Inst{11-0} = Disp;
}

class FullLoadCondDisp9Shift2<bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rp, uimm9shift2:$Disp),
                  Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<9> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rd;
  let Inst{24-20} = 0b11111;
  let Inst{19-16} = Rp;
  let Inst{15-12} = Cond;
  let Inst{11-9} = 0b000;
  let Inst{8-0} = Disp;
}

class FullMemBit<bits<12> Prefix, string Mnemonic>
    : AVR32Inst32<(outs), (ins simm15shift2:$Imm, uimm5:$Bp),
                  Mnemonic#"\t$Imm, $Bp"> {
  bits<15> Imm;
  bits<5> Bp;

  let Inst{31-20} = Prefix;
  let Inst{19-16} = Bp{4-1};
  let Inst{15} = Bp{0};
  let Inst{14-0} = Imm;
}

class FullLoadCondDisp9<bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rp, uimm9:$Disp),
                  Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<9> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rd;
  let Inst{24-20} = 0b11111;
  let Inst{19-16} = Rp;
  let Inst{15-12} = Cond;
  let Inst{11-9} = 0b100;
  let Inst{8-0} = Disp;
}

class FullLoadCondDisp9Sub<bits<4> Cond, bits<3> Sub, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rp, uimm9:$Disp),
                  Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<9> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rd;
  let Inst{24-20} = 0b11111;
  let Inst{19-16} = Rp;
  let Inst{15-12} = Cond;
  let Inst{11-9} = Sub;
  let Inst{8-0} = Disp;
}

class FullLoadCondDisp9Shift1<bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rp, uimm9shift1:$Disp),
                  Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<9> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rd;
  let Inst{24-20} = 0b11111;
  let Inst{19-16} = Rp;
  let Inst{15-12} = Cond;
  let Inst{11-9} = 0b001;
  let Inst{8-0} = Disp;
}

class FullLoadCondDisp9Shift1Sub<bits<4> Cond, bits<3> Sub, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rp, uimm9shift1:$Disp),
                  Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<9> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rd;
  let Inst{24-20} = 0b11111;
  let Inst{19-16} = Rp;
  let Inst{15-12} = Cond;
  let Inst{11-9} = Sub;
  let Inst{8-0} = Disp;
}

class FullStoreHalfwordPairDisp8
    : AVR32Inst32<(outs), (ins GPR:$Rp, uimm8shift2:$Disp, GPR:$Rx,
                               halfpart:$XPart, GPR:$Ry, halfpart:$YPart),
                  "sthh.w\t${Rp}[$Disp], ${Rx}:$XPart, ${Ry}:$YPart"> {
  let mayStore = 1;

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
  let mayStore = 1;

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
  let mayStore = 1;

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

class FullLoadSwapDisp12<bits<4> Prefix, string Mnemonic, Operand DispOp>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rp, DispOp:$Disp),
                  Mnemonic#"\t$Rd, ${Rp}[$Disp]"> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;
  bits<12> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rp;
  let Inst{24-20} = 0b11101;
  let Inst{19-16} = Rd;
  let Inst{15-12} = Prefix;
  let Inst{11-0} = Disp;
}

class FullStorePairDisp16<bits<5> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, simm16:$Disp, GPREven:$Rs),
                  Mnemonic#"\t${Rp}[$Disp], $Rs"> {
  let mayStore = 1;

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
  let mayStore = 1;

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
  let mayStore = 1;

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
  let mayStore = 1;

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
  let mayStore = 1;

  bits<4> Rp;
  bits<4> Rs;

  let Inst{15-13} = 0b101;
  let Inst{12-9} = Rp;
  let Inst{8-4} = Op;
  let Inst{3-1} = Rs{3-1};
  let Inst{0} = LastBit;
}

class CompactLoadPair<bits<5> Op, bits<1> LastBit, string AsmStr>
    : AVR32Inst<0, (outs GPREven:$Rd), (ins GPR:$Rp), AsmStr> {
  let mayLoad = 1;

  bits<4> Rd;
  bits<4> Rp;

  let Inst{15-13} = 0b101;
  let Inst{12-9} = Rp;
  let Inst{8-4} = Op;
  let Inst{3-1} = Rd{3-1};
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

class CompactRegRegWithPrefixCG<bits<3> Prefix, bits<5> Op, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Old, GPR:$Rs),
                Mnemonic#"\t$Rd, $Rs"> {
  let Defs = [SREG];

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

class CompactRegUImm5Shift<bits<3> Op, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins uimm5:$Sa),
                Mnemonic#"\t$Rd, $Sa"> {
  let Defs = [SREG];

  bits<4> Rd;
  bits<5> Sa;

  let Inst{15-13} = 0b101;
  let Inst{12} = Sa{4};
  let Inst{11-9} = Sa{3-1};
  let Inst{8} = 1;
  let Inst{7-5} = Op;
  let Inst{4} = Sa{0};
  let Inst{3-0} = Rd;
}

class CompactRegUImm5ShiftCG<bits<3> Op, string Mnemonic>
    : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Old, uimm5:$Sa),
                Mnemonic#"\t$Rd, $Sa"> {
  let Defs = [SREG];

  bits<4> Rd;
  bits<5> Sa;

  let Inst{15-13} = 0b101;
  let Inst{12} = Sa{4};
  let Inst{11-9} = Sa{3-1};
  let Inst{8} = 1;
  let Inst{7-5} = Op;
  let Inst{4} = Sa{0};
  let Inst{3-0} = Rd;
}

class CompactRegSImm8<bits<4> Prefix, string Mnemonic>
    : AVR32Inst<0, (outs GPRNoSP:$Rd), (ins simm8:$Imm),
                Mnemonic#"\t$Rd, $Imm"> {
  let Defs = [SREG];

  bits<4> Rd;
  bits<8> Imm;

  let Inst{15-12} = Prefix;
  let Inst{11-4} = Imm;
  let Inst{3-0} = Rd;
}

class CompactSPSubImm<string Mnemonic>
    : AVR32Inst<0, (outs SPOnly:$Rd), (ins subspimm:$Imm),
                Mnemonic#"\t$Rd, $Imm"> {
  let Defs = [SREG];

  bits<4> Rd;
  bits<8> Imm;

  let Inst{15-12} = 0b0010;
  let Inst{11-4} = Imm;
  let Inst{3-0} = Rd;
}

class CompactSPStoreDisp7Shift2
    : AVR32Inst<0, (outs), (ins SPOnly:$Rp, uimm7shift2:$Disp, GPR:$Rs),
                "stdsp\t${Rp}[$Disp], $Rs"> {
  let mayStore = 1;

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

class CompactPCRel10<bits<2> Op, string Mnemonic>
    : AVR32Inst<0, (outs), (ins PCOnly:$Rp, simm10shift1:$Disp),
                Mnemonic#"\t${Rp}[$Disp]"> {
  bits<10> Disp;

  let Inst{15-12} = 0b1100;
  let Inst{11-4} = Disp{7-0};
  let Inst{3-2} = Op;
  let Inst{1-0} = Disp{9-8};
}

class CompactRjmp<string Mnemonic>
    : AVR32Inst<0, (outs), (ins PCOnly:$Rp, simm10pcrelshift1:$Disp),
                Mnemonic#"\t${Rp}[$Disp]"> {
  bits<10> Disp;

  let Inst{15-12} = 0b1100;
  let Inst{11-4} = Disp{7-0};
  let Inst{3-2} = 0b10;
  let Inst{1-0} = Disp{9-8};
}

class CompactRjmpCG<string Mnemonic>
    : AVR32Inst<0, (outs), (ins brtarget11:$Disp), Mnemonic#"\t$Disp"> {
  bits<10> Disp;

  let Inst{15-12} = 0b1100;
  let Inst{11-4} = Disp{7-0};
  let Inst{3-2} = 0b10;
  let Inst{1-0} = Disp{9-8};
  let isCodeGenOnly = 1;
}

class CompactBranch<bits<3> Cond, string Mnemonic>
    : AVR32Inst<0, (outs), (ins simm8shift1:$Disp), Mnemonic#"\t$Disp"> {
  bits<8> Disp;

  let Inst{15-12} = 0b1100;
  let Inst{11-4} = Disp;
  let Inst{3} = 0;
  let Inst{2-0} = Cond;
}

class CompactBranchCG<bits<3> Cond, string Mnemonic>
    : AVR32Inst<0, (outs), (ins brtarget9:$Disp), Mnemonic#"\t$Disp"> {
  bits<8> Disp;

  let Inst{15-12} = 0b1100;
  let Inst{11-4} = Disp;
  let Inst{3} = 0;
  let Inst{2-0} = Cond;
  let isCodeGenOnly = 1;
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

class FullPCRel21<string Mnemonic>
    : AVR32Inst32<(outs), (ins PCOnly:$Rp, simm21pcrelshift1:$Disp),
                  Mnemonic#"\t${Rp}[$Disp]"> {
  bits<21> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Disp{20-17};
  let Inst{24-21} = 0b0101;
  let Inst{20} = Disp{16};
  let Inst{19-16} = 0;
  let Inst{15-0} = Disp{15-0};
}

class FullPCRel21CG<string Mnemonic>
    : AVR32Inst32<(outs), (ins simm21pcrelshift1:$Disp), Mnemonic#"\t$Disp"> {
  bits<21> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Disp{20-17};
  let Inst{24-21} = 0b0101;
  let Inst{20} = Disp{16};
  let Inst{19-16} = 0;
  let Inst{15-0} = Disp{15-0};
  let isCodeGenOnly = 1;
}

class FullBranch<bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs), (ins simm21pcrelshift1:$Disp),
                  Mnemonic#"\t$Disp"> {
  bits<21> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Disp{20-17};
  let Inst{24-21} = 0b0100;
  let Inst{20} = Disp{16};
  let Inst{19-16} = Cond;
  let Inst{15-0} = Disp{15-0};
}

class FullBranchCG<bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs), (ins brtarget22:$Disp), Mnemonic#"\t$Disp"> {
  bits<21> Disp;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Disp{20-17};
  let Inst{24-21} = 0b0100;
  let Inst{20} = Disp{16};
  let Inst{19-16} = Cond;
  let Inst{15-0} = Disp{15-0};
  let isCodeGenOnly = 1;
}

class FullRegList<bits<3> Op, bit Update, string Asm>
    : AVR32Inst32<(outs), (ins GPR:$Rp, reglist16:$Mask), Asm> {
  bits<4> Rp;
  bits<16> Mask;

  let Inst{31-29} = 0b111;
  let Inst{28-26} = Op;
  let Inst{25} = Update;
  let Inst{24-20} = 0b11100;
  let Inst{19-16} = Rp;
  let Inst{15-0} = Mask;
}

class CompactRegList8<bits<4> Low, string Mnemonic>
    : AVR32Inst<0, (outs), (ins reglist8:$Mask), Mnemonic#"\t$Mask"> {
  bits<8> Mask;

  let Inst{15-12} = 0b1101;
  let Inst{11} = Mask{7};
  let Inst{10} = Mask{6};
  let Inst{9} = Mask{5};
  let Inst{8} = Mask{4};
  let Inst{7} = Mask{3};
  let Inst{6} = Mask{2};
  let Inst{5} = Mask{1};
  let Inst{4} = Mask{0};
  let Inst{3-0} = Low;
}

class CompactPopmRetVal
    : AVR32Inst<0, (outs), (ins reglist8:$Mask, popmretval:$RetVal),
                "popm\t$Mask, r12=$RetVal"> {
  bits<8> Mask;
  bits<2> RetVal;

  let Inst{15-12} = 0b1101;
  let Inst{11} = Mask{7};
  let Inst{10-9} = RetVal;
  let Inst{8} = Mask{4};
  let Inst{7} = Mask{3};
  let Inst{6} = Mask{2};
  let Inst{5} = Mask{1};
  let Inst{4} = Mask{0};
  let Inst{3-0} = 0b1010;
}

class MoveCopRegToRegD<string Mnemonic>
    : AVR32Inst32<(outs GPREven:$Rd),
                  (ins coprocessor:$CP, coprocessorevenreg:$CRs),
                  Mnemonic#"\t$CP, $Rd, $CRs"> {
  bits<4> Rd;
  bits<3> CP;
  bits<4> CRs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = 0b0111;
  let Inst{24-20} = 0b11010;
  let Inst{19-17} = Rd{3-1};
  let Inst{16} = 0;
  let Inst{15-13} = CP;
  let Inst{12} = 0;
  let Inst{11-9} = CRs{3-1};
  let Inst{8} = 0;
  let Inst{7-0} = 0b00010000;
}

class MoveCopRegToRegW<string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd),
                  (ins coprocessor:$CP, coprocessorreg:$CRs),
                  Mnemonic#"\t$CP, $Rd, $CRs"> {
  bits<4> Rd;
  bits<3> CP;
  bits<4> CRs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = 0b0111;
  let Inst{24-20} = 0b11010;
  let Inst{19-16} = Rd;
  let Inst{15-13} = CP;
  let Inst{12} = 0;
  let Inst{11-8} = CRs;
  let Inst{7-0} = 0;
}

class MoveRegToCopRegD<string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, coprocessorevenreg:$CRd, GPREven:$Rs),
                  Mnemonic#"\t$CP, $CRd, $Rs"> {
  bits<3> CP;
  bits<4> CRd;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = 0b0111;
  let Inst{24-20} = 0b11010;
  let Inst{19-17} = Rs{3-1};
  let Inst{16} = 0;
  let Inst{15-13} = CP;
  let Inst{12} = 0;
  let Inst{11-9} = CRd{3-1};
  let Inst{8} = 0;
  let Inst{7-0} = 0b00110000;
}

class MoveRegToCopRegW<string Mnemonic>
    : AVR32Inst32<(outs), (ins coprocessor:$CP, coprocessorreg:$CRd, GPR:$Rs),
                  Mnemonic#"\t$CP, $CRd, $Rs"> {
  bits<3> CP;
  bits<4> CRd;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = 0b0111;
  let Inst{24-20} = 0b11010;
  let Inst{19-16} = Rs;
  let Inst{15-13} = CP;
  let Inst{12} = 0;
  let Inst{11-8} = CRd;
  let Inst{7-0} = 0b00100000;
}

class PicoMoveCopRegToRegD<string Mnemonic>
    : AVR32Inst32<(outs GPREven:$Rd), (ins picoevenreg:$CRs),
                  Mnemonic#"\t$Rd, $CRs"> {
  bits<4> Rd;
  bits<4> CRs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = 0b0111;
  let Inst{24-20} = 0b11010;
  let Inst{19-17} = Rd{3-1};
  let Inst{16} = 0;
  let Inst{15-13} = 0b001;
  let Inst{12} = 0;
  let Inst{11-9} = CRs{3-1};
  let Inst{8} = 0;
  let Inst{7-0} = 0b00010000;
}

class PicoMoveCopRegToRegW<string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins picoreg:$CRs),
                  Mnemonic#"\t$Rd, $CRs"> {
  bits<4> Rd;
  bits<4> CRs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = 0b0111;
  let Inst{24-20} = 0b11010;
  let Inst{19-16} = Rd;
  let Inst{15-13} = 0b001;
  let Inst{12} = 0;
  let Inst{11-8} = CRs;
  let Inst{7-0} = 0;
}

class PicoMoveRegToCopRegD<string Mnemonic>
    : AVR32Inst32<(outs), (ins picoevenreg:$CRd, GPREven:$Rs),
                  Mnemonic#"\t$CRd, $Rs"> {
  bits<4> CRd;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = 0b0111;
  let Inst{24-20} = 0b11010;
  let Inst{19-17} = Rs{3-1};
  let Inst{16} = 0;
  let Inst{15-13} = 0b001;
  let Inst{12} = 0;
  let Inst{11-9} = CRd{3-1};
  let Inst{8} = 0;
  let Inst{7-0} = 0b00110000;
}

class PicoMoveRegToCopRegW<string Mnemonic>
    : AVR32Inst32<(outs), (ins picoreg:$CRd, GPR:$Rs),
                  Mnemonic#"\t$CRd, $Rs"> {
  bits<4> CRd;
  bits<4> Rs;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = 0b0111;
  let Inst{24-20} = 0b11010;
  let Inst{19-16} = Rs;
  let Inst{15-13} = 0b001;
  let Inst{12} = 0;
  let Inst{11-8} = CRd;
  let Inst{7-0} = 0b00100000;
}

class CoprocessorOp<string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, coprocessorreg:$CRd,
                       coprocessorreg:$CRx, coprocessorreg:$CRy, uimm7:$Op),
                  Mnemonic#"\t$CP, $CRd, $CRx, $CRy, $Op"> {
  bits<3> CP;
  bits<4> CRd;
  bits<4> CRx;
  bits<4> CRy;
  bits<7> Op;

  let Inst{31-27} = 0b11100;
  let Inst{26-25} = Op{5-4};
  let Inst{24-20} = 0b11010;
  let Inst{19-16} = Op{3-0};
  let Inst{15-13} = CP;
  let Inst{12} = Op{6};
  let Inst{11-8} = CRd;
  let Inst{7-4} = CRx;
  let Inst{3-0} = CRy;
}

class PicoCoprocessorOp<bits<7> Op, string Output, string Mnemonic>
    : AVR32Inst32<(outs), (ins picoin:$CRd, picoin:$CRx, picoin:$CRy),
                  Mnemonic#"\t"#Output#", $CRd, $CRx, $CRy"> {
  bits<4> CRd;
  bits<4> CRx;
  bits<4> CRy;

  let Inst{31-27} = 0b11100;
  let Inst{26-25} = Op{5-4};
  let Inst{24-20} = 0b11010;
  let Inst{19-16} = Op{3-0};
  let Inst{15-13} = 0b001;
  let Inst{12} = Op{6};
  let Inst{11-8} = CRd;
  let Inst{7-4} = CRx;
  let Inst{3-0} = CRy;
}

class CopLoadDispD<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, coprocessorevenreg:$CRd, GPR:$Rp,
                       uimm8shift2:$Disp),
                  Mnemonic#"\t$CP, $CRd, ${Rp}[$Disp]"> {
  bits<3> CP;
  bits<4> CRd;
  bits<4> Rp;
  bits<8> Disp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-13} = CP;
  let Inst{12} = 1;
  let Inst{11-9} = CRd{3-1};
  let Inst{8} = 0;
  let Inst{7-0} = Disp;
}

class CopLoadDispW<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, coprocessorreg:$CRd, GPR:$Rp,
                       uimm8shift2:$Disp),
                  Mnemonic#"\t$CP, $CRd, ${Rp}[$Disp]"> {
  bits<3> CP;
  bits<4> CRd;
  bits<4> Rp;
  bits<8> Disp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-13} = CP;
  let Inst{12} = 0;
  let Inst{11-8} = CRd;
  let Inst{7-0} = Disp;
}

class PicoLoadDispD<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins picoevenreg:$CRd, GPR:$Rp, uimm8shift2:$Disp),
                  Mnemonic#"\t$CRd, ${Rp}[$Disp]"> {
  bits<4> CRd;
  bits<4> Rp;
  bits<8> Disp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-13} = 0b001;
  let Inst{12} = 1;
  let Inst{11-9} = CRd{3-1};
  let Inst{8} = 0;
  let Inst{7-0} = Disp;
}

class PicoLoadDispW<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins picoreg:$CRd, GPR:$Rp, uimm8shift2:$Disp),
                  Mnemonic#"\t$CRd, ${Rp}[$Disp]"> {
  bits<4> CRd;
  bits<4> Rp;
  bits<8> Disp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-13} = 0b001;
  let Inst{12} = 0;
  let Inst{11-8} = CRd;
  let Inst{7-0} = Disp;
}

class CopLoadPreDecD<bits<9> Low, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, coprocessorevenreg:$CRd, GPR:$Rp),
                  Mnemonic#"\t$CP, $CRd, --$Rp"> {
  bits<3> CP;
  bits<4> CRd;
  bits<4> Rp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = CP;
  let Inst{12} = 0;
  let Inst{11-9} = CRd{3-1};
  let Inst{8-0} = Low;
}

class CopLoadPreDecW<bits<8> Low, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, coprocessorreg:$CRd, GPR:$Rp),
                  Mnemonic#"\t$CP, $CRd, --$Rp"> {
  bits<3> CP;
  bits<4> CRd;
  bits<4> Rp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = CP;
  let Inst{12} = 0;
  let Inst{11-8} = CRd;
  let Inst{7-0} = Low;
}

class PicoLoadPreDecD<bits<9> Low, string Mnemonic>
    : AVR32Inst32<(outs), (ins picoevenreg:$CRd, GPR:$Rp),
                  Mnemonic#"\t$CRd, --$Rp"> {
  bits<4> CRd;
  bits<4> Rp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = 0b001;
  let Inst{12} = 0;
  let Inst{11-9} = CRd{3-1};
  let Inst{8-0} = Low;
}

class PicoLoadPreDecW<bits<8> Low, string Mnemonic>
    : AVR32Inst32<(outs), (ins picoreg:$CRd, GPR:$Rp),
                  Mnemonic#"\t$CRd, --$Rp"> {
  bits<4> CRd;
  bits<4> Rp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = 0b001;
  let Inst{12} = 0;
  let Inst{11-8} = CRd;
  let Inst{7-0} = Low;
}

class CopLoadIndexShiftD<string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, coprocessorevenreg:$CRd, GPR:$Rb,
                       GPR:$Ri, uimm2:$Shift),
                  Mnemonic#"\t$CP, $CRd, ${Rb}[$Ri << $Shift]"> {
  bits<3> CP;
  bits<4> CRd;
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rb;
  let Inst{15-13} = CP;
  let Inst{12} = 1;
  let Inst{11-9} = CRd{3-1};
  let Inst{8} = 0;
  let Inst{7-6} = 0b01;
  let Inst{5-4} = Shift;
  let Inst{3-0} = Ri;
}

class CopLoadIndexShiftW<string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, coprocessorreg:$CRd, GPR:$Rb,
                       GPR:$Ri, uimm2:$Shift),
                  Mnemonic#"\t$CP, $CRd, ${Rb}[$Ri << $Shift]"> {
  bits<3> CP;
  bits<4> CRd;
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rb;
  let Inst{15-13} = CP;
  let Inst{12} = 1;
  let Inst{11-8} = CRd;
  let Inst{7-6} = 0b00;
  let Inst{5-4} = Shift;
  let Inst{3-0} = Ri;
}

class PicoLoadIndexShiftD<string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins picoevenreg:$CRd, GPR:$Rb, GPR:$Ri, uimm2:$Shift),
                  Mnemonic#"\t$CRd, ${Rb}[$Ri << $Shift]"> {
  bits<4> CRd;
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rb;
  let Inst{15-13} = 0b001;
  let Inst{12} = 1;
  let Inst{11-9} = CRd{3-1};
  let Inst{8} = 0;
  let Inst{7-6} = 0b01;
  let Inst{5-4} = Shift;
  let Inst{3-0} = Ri;
}

class PicoLoadIndexShiftW<string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins picoreg:$CRd, GPR:$Rb, GPR:$Ri, uimm2:$Shift),
                  Mnemonic#"\t$CRd, ${Rb}[$Ri << $Shift]"> {
  bits<4> CRd;
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rb;
  let Inst{15-13} = 0b001;
  let Inst{12} = 1;
  let Inst{11-8} = CRd;
  let Inst{7-6} = 0b00;
  let Inst{5-4} = Shift;
  let Inst{3-0} = Ri;
}

class Cop0LoadDispD<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessorevenreg:$CRd, GPR:$Rp,
                       uimm12shift2:$Disp),
                  Mnemonic#"\t$CRd, ${Rp}[$Disp]"> {
  bits<4> CRd;
  bits<4> Rp;
  bits<12> Disp;

  let Inst{31-27} = 0b11110;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-12} = Disp{11-8};
  let Inst{11-9} = CRd{3-1};
  let Inst{8} = 0;
  let Inst{7-0} = Disp{7-0};
}

class Cop0LoadDispW<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessorreg:$CRd, GPR:$Rp, uimm12shift2:$Disp),
                  Mnemonic#"\t$CRd, ${Rp}[$Disp]"> {
  bits<4> CRd;
  bits<4> Rp;
  bits<12> Disp;

  let Inst{31-27} = 0b11110;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-12} = Disp{11-8};
  let Inst{11-8} = CRd;
  let Inst{7-0} = Disp{7-0};
}

class CopStoreDispD<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, GPR:$Rp, uimm8shift2:$Disp,
                       coprocessorevenreg:$CRs),
                  Mnemonic#"\t$CP, ${Rp}[$Disp], $CRs"> {
  bits<3> CP;
  bits<4> CRs;
  bits<4> Rp;
  bits<8> Disp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-13} = CP;
  let Inst{12} = 1;
  let Inst{11-9} = CRs{3-1};
  let Inst{8} = 0;
  let Inst{7-0} = Disp;
}

class CopStoreDispW<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, GPR:$Rp, uimm8shift2:$Disp,
                       coprocessorreg:$CRs),
                  Mnemonic#"\t$CP, ${Rp}[$Disp], $CRs"> {
  bits<3> CP;
  bits<4> CRs;
  bits<4> Rp;
  bits<8> Disp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-13} = CP;
  let Inst{12} = 0;
  let Inst{11-8} = CRs;
  let Inst{7-0} = Disp;
}

class PicoStoreDispD<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, uimm8shift2:$Disp, picoevenreg:$CRs),
                  Mnemonic#"\t${Rp}[$Disp], $CRs"> {
  bits<4> CRs;
  bits<4> Rp;
  bits<8> Disp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-13} = 0b001;
  let Inst{12} = 1;
  let Inst{11-9} = CRs{3-1};
  let Inst{8} = 0;
  let Inst{7-0} = Disp;
}

class PicoStoreDispW<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, uimm8shift2:$Disp, picoreg:$CRs),
                  Mnemonic#"\t${Rp}[$Disp], $CRs"> {
  bits<4> CRs;
  bits<4> Rp;
  bits<8> Disp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-13} = 0b001;
  let Inst{12} = 0;
  let Inst{11-8} = CRs;
  let Inst{7-0} = Disp;
}

class CopStorePostIncD<bits<9> Low, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, GPR:$Rp, coprocessorevenreg:$CRs),
                  Mnemonic#"\t$CP, ${Rp}++, $CRs"> {
  bits<3> CP;
  bits<4> CRs;
  bits<4> Rp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = CP;
  let Inst{12} = 0;
  let Inst{11-9} = CRs{3-1};
  let Inst{8-0} = Low;
}

class CopStorePostIncW<bits<8> Low, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, GPR:$Rp, coprocessorreg:$CRs),
                  Mnemonic#"\t$CP, ${Rp}++, $CRs"> {
  bits<3> CP;
  bits<4> CRs;
  bits<4> Rp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = CP;
  let Inst{12} = 0;
  let Inst{11-8} = CRs;
  let Inst{7-0} = Low;
}

class PicoStorePostIncD<bits<9> Low, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, picoevenreg:$CRs),
                  Mnemonic#"\t${Rp}++, $CRs"> {
  bits<4> CRs;
  bits<4> Rp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = 0b001;
  let Inst{12} = 0;
  let Inst{11-9} = CRs{3-1};
  let Inst{8-0} = Low;
}

class PicoStorePostIncW<bits<8> Low, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rp, picoreg:$CRs),
                  Mnemonic#"\t${Rp}++, $CRs"> {
  bits<4> CRs;
  bits<4> Rp;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = 0b001;
  let Inst{12} = 0;
  let Inst{11-8} = CRs;
  let Inst{7-0} = Low;
}

class CopStoreIndexShiftD<string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, GPR:$Rb, GPR:$Ri, uimm2:$Shift,
                       coprocessorevenreg:$CRs),
                  Mnemonic#"\t$CP, ${Rb}[$Ri << $Shift], $CRs"> {
  bits<3> CP;
  bits<4> CRs;
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rb;
  let Inst{15-13} = CP;
  let Inst{12} = 1;
  let Inst{11-9} = CRs{3-1};
  let Inst{8} = 0;
  let Inst{7-6} = 0b11;
  let Inst{5-4} = Shift;
  let Inst{3-0} = Ri;
}

class CopStoreIndexShiftW<string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins coprocessor:$CP, GPR:$Rb, GPR:$Ri, uimm2:$Shift,
                       coprocessorreg:$CRs),
                  Mnemonic#"\t$CP, ${Rb}[$Ri << $Shift], $CRs"> {
  bits<3> CP;
  bits<4> CRs;
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rb;
  let Inst{15-13} = CP;
  let Inst{12} = 1;
  let Inst{11-8} = CRs;
  let Inst{7-6} = 0b10;
  let Inst{5-4} = Shift;
  let Inst{3-0} = Ri;
}

class PicoStoreIndexShiftD<string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins GPR:$Rb, GPR:$Ri, uimm2:$Shift, picoevenreg:$CRs),
                  Mnemonic#"\t${Rb}[$Ri << $Shift], $CRs"> {
  bits<4> CRs;
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rb;
  let Inst{15-13} = 0b001;
  let Inst{12} = 1;
  let Inst{11-9} = CRs{3-1};
  let Inst{8} = 0;
  let Inst{7-6} = 0b11;
  let Inst{5-4} = Shift;
  let Inst{3-0} = Ri;
}

class PicoStoreIndexShiftW<string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins GPR:$Rb, GPR:$Ri, uimm2:$Shift, picoreg:$CRs),
                  Mnemonic#"\t${Rb}[$Ri << $Shift], $CRs"> {
  bits<4> CRs;
  bits<4> Rb;
  bits<4> Ri;
  bits<2> Shift;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1111010;
  let Inst{19-16} = Rb;
  let Inst{15-13} = 0b001;
  let Inst{12} = 1;
  let Inst{11-8} = CRs;
  let Inst{7-6} = 0b10;
  let Inst{5-4} = Shift;
  let Inst{3-0} = Ri;
}

class Cop0StoreDispD<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins GPR:$Rp, uimm12shift2:$Disp,
                       coprocessorevenreg:$CRs),
                  Mnemonic#"\t${Rp}[$Disp], $CRs"> {
  bits<4> CRs;
  bits<4> Rp;
  bits<12> Disp;

  let Inst{31-27} = 0b11110;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-12} = Disp{11-8};
  let Inst{11-9} = CRs{3-1};
  let Inst{8} = 0;
  let Inst{7-0} = Disp{7-0};
}

class Cop0StoreDispW<bits<7> Op, string Mnemonic>
    : AVR32Inst32<(outs),
                  (ins GPR:$Rp, uimm12shift2:$Disp, coprocessorreg:$CRs),
                  Mnemonic#"\t${Rp}[$Disp], $CRs"> {
  bits<4> CRs;
  bits<4> Rp;
  bits<12> Disp;

  let Inst{31-27} = 0b11110;
  let Inst{26-20} = Op;
  let Inst{19-16} = Rp;
  let Inst{15-12} = Disp{11-8};
  let Inst{11-8} = CRs;
  let Inst{7-0} = Disp{7-0};
}

class CopLoadMultiple<bits<4> Sub, bit Update, Operand RegListOp,
                      string Asm>
    : AVR32Inst32<(outs), (ins coprocessor:$CP, GPR:$Rp, RegListOp:$Mask),
                  Asm> {
  bits<3> CP;
  bits<4> Rp;
  bits<16> Mask;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1011010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = CP;
  let Inst{12} = Update;
  let Inst{11-8} = Sub;
}

class CopLoadMultipleLow<bits<4> Sub, bit Update, Operand RegListOp,
                         string Asm>
    : CopLoadMultiple<Sub, Update, RegListOp, Asm> {
  let Inst{7-0} = Mask{7-0};
}

class CopLoadMultipleHigh<bits<4> Sub, bit Update, string Asm>
    : CopLoadMultiple<Sub, Update, coprocessorreglisthigh, Asm> {
  let Inst{7-0} = Mask{15-8};
}

class PicoLoadMultiple<bits<4> Sub, bit Update, Operand RegListOp, string Asm>
    : AVR32Inst32<(outs), (ins GPR:$Rp, RegListOp:$Mask), Asm> {
  bits<4> Rp;
  bits<16> Mask;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1011010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = 0b001;
  let Inst{12} = Update;
  let Inst{11-8} = Sub;
}

class PicoLoadMultipleLow<bits<4> Sub, bit Update, Operand RegListOp,
                          string Asm>
    : PicoLoadMultiple<Sub, Update, RegListOp, Asm> {
  let Inst{7-0} = Mask{7-0};
}

class PicoLoadMultipleHigh<bits<4> Sub, bit Update, string Asm>
    : PicoLoadMultiple<Sub, Update, picoreglisthigh, Asm> {
  let Inst{7-0} = Mask{15-8};
}

class CopStoreMultiple<bits<4> Sub, bit Update, Operand RegListOp,
                       string Asm>
    : AVR32Inst32<(outs), (ins coprocessor:$CP, GPR:$Rp, RegListOp:$Mask),
                  Asm> {
  bits<3> CP;
  bits<4> Rp;
  bits<16> Mask;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1011010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = CP;
  let Inst{12} = Update;
  let Inst{11-8} = Sub;
}

class CopStoreMultipleLow<bits<4> Sub, bit Update, Operand RegListOp,
                          string Asm>
    : CopStoreMultiple<Sub, Update, RegListOp, Asm> {
  let Inst{7-0} = Mask{7-0};
}

class CopStoreMultipleHigh<bits<4> Sub, bit Update, string Asm>
    : CopStoreMultiple<Sub, Update, coprocessorreglisthigh, Asm> {
  let Inst{7-0} = Mask{15-8};
}

class PicoStoreMultiple<bits<4> Sub, bit Update, Operand RegListOp, string Asm>
    : AVR32Inst32<(outs), (ins GPR:$Rp, RegListOp:$Mask), Asm> {
  bits<4> Rp;
  bits<16> Mask;

  let Inst{31-27} = 0b11101;
  let Inst{26-20} = 0b1011010;
  let Inst{19-16} = Rp;
  let Inst{15-13} = 0b001;
  let Inst{12} = Update;
  let Inst{11-8} = Sub;
}

class PicoStoreMultipleLow<bits<4> Sub, bit Update, Operand RegListOp,
                           string Asm>
    : PicoStoreMultiple<Sub, Update, RegListOp, Asm> {
  let Inst{7-0} = Mask{7-0};
}

class PicoStoreMultipleHigh<bits<4> Sub, bit Update, string Asm>
    : PicoStoreMultiple<Sub, Update, picoreglisthigh, Asm> {
  let Inst{7-0} = Mask{15-8};
}

def LD_W_PostInc : CompactLoadPostInc<0b10000, "ld.w">;
def LD_W_PreDec : CompactLoadPreDec<0b10100, "ld.w">;
def LD_W_Disp5 : CompactLoadDisp5Shift2<"ld.w">;
def LD_W_Disp16 : FullLoadDisp16<0b01111, "ld.w">;
def LD_W_IndexShift : FullLoadIndexShift<0b0000001100, "ld.w">;
def LD_W_JT : FullLoadIndexShiftPC<0b0000001100, "ld.w">;
def LD_W_IndexPart : FullLoadIndexPart<"ld.w">;
def LD_W_AL_Disp9 : FullLoadCondDisp9Shift2<0b1111, "ld.wal">;
def LD_W_EQ_Disp9 : FullLoadCondDisp9Shift2<0b0000, "ld.weq">;
def LD_W_NE_Disp9 : FullLoadCondDisp9Shift2<0b0001, "ld.wne">;
def LD_W_CC_Disp9 : FullLoadCondDisp9Shift2<0b0010, "ld.wcc">;
def LD_W_HS_Disp9 : FullLoadCondDisp9Shift2<0b0010, "ld.whs">;
def LD_W_CS_Disp9 : FullLoadCondDisp9Shift2<0b0011, "ld.wcs">;
def LD_W_LO_Disp9 : FullLoadCondDisp9Shift2<0b0011, "ld.wlo">;
def LD_W_GE_Disp9 : FullLoadCondDisp9Shift2<0b0100, "ld.wge">;
def LD_W_LT_Disp9 : FullLoadCondDisp9Shift2<0b0101, "ld.wlt">;
def LD_W_MI_Disp9 : FullLoadCondDisp9Shift2<0b0110, "ld.wmi">;
def LD_W_PL_Disp9 : FullLoadCondDisp9Shift2<0b0111, "ld.wpl">;
def LD_W_LS_Disp9 : FullLoadCondDisp9Shift2<0b1000, "ld.wls">;
def LD_W_GT_Disp9 : FullLoadCondDisp9Shift2<0b1001, "ld.wgt">;
def LD_W_LE_Disp9 : FullLoadCondDisp9Shift2<0b1010, "ld.wle">;
def LD_W_HI_Disp9 : FullLoadCondDisp9Shift2<0b1011, "ld.whi">;
def LD_W_VS_Disp9 : FullLoadCondDisp9Shift2<0b1100, "ld.wvs">;
def LD_W_VC_Disp9 : FullLoadCondDisp9Shift2<0b1101, "ld.wvc">;
def LD_W_QS_Disp9 : FullLoadCondDisp9Shift2<0b1110, "ld.wqs">;
def LDC_D_Disp : CopLoadDispD<0b0011010, "ldc.d">;
def LDC_D_PreDec : CopLoadPreDecD<0b001010000, "ldc.d">;
def LDC_D_IndexShift : CopLoadIndexShiftD<"ldc.d">;
def LDC_W_Disp : CopLoadDispW<0b0011010, "ldc.w">;
def LDC_W_PreDec : CopLoadPreDecW<0b01000000, "ldc.w">;
def LDC_W_IndexShift : CopLoadIndexShiftW<"ldc.w">;
def PICOLD_D_Disp : PicoLoadDispD<0b0011010, "picold.d">;
def PICOLD_D_PreDec : PicoLoadPreDecD<0b001010000, "picold.d">;
def PICOLD_D_IndexShift : PicoLoadIndexShiftD<"picold.d">;
def PICOLD_W_Disp : PicoLoadDispW<0b0011010, "picold.w">;
def PICOLD_W_PreDec : PicoLoadPreDecW<0b01000000, "picold.w">;
def PICOLD_W_IndexShift : PicoLoadIndexShiftW<"picold.w">;
def LDC0_D_Disp : Cop0LoadDispD<0b0111010, "ldc0.d">;
def LDC0_W_Disp : Cop0LoadDispW<0b0011010, "ldc0.w">;
def LD_D_PostInc : CompactLoadPair<0b10000, 1, "ld.d\t$Rd, ${Rp}++">;
def LD_D_PreDec : CompactLoadPair<0b10001, 0, "ld.d\t$Rd, --$Rp">;
def LD_D_Reg : CompactLoadPair<0b10000, 0, "ld.d\t$Rd, $Rp">;
def LD_D_Disp16 : FullLoadPairDisp16<0b01110, "ld.d">;
def LD_D_IndexShift : FullLoadPairIndexShift<0b0000001000, "ld.d">;
let Size = 4 in
def LDA_W : AVR32Pseudo<(outs GPR:$Rd), (ins ldaexpr:$Imm),
                        "lda.w\t$Rd, $Imm">;
def LDDPC : CompactLoadBaseDisp7Shift2<0b01001, PCOnly, "lddpc">;
def LDDPCp : AVR32Pseudo<(outs GPR:$Rd),
                         (ins PCOnly:$Rp, simm16pcrel:$Disp),
                         "lddpc\t$Rd, ${Rp}[$Disp]">;
def LDDPC_PCREL16 : FullLoadPCRel16<0b01111, "lddpc">;
def LDDPCcp : CompactLoadBasePCRel7Shift2<0b01001, PCOnly, "lddpc">;
def LDDSP : CompactLoadBaseDisp7Shift2<0b01000, SPOnly, "lddsp">;
def LDINS_B : FullLoadInsertByteDisp12;
def LDINS_H : FullLoadInsertHalfDisp12;
def LDSWP_SH_Disp12 : FullLoadSwapDisp12<0b0010, "ldswp.sh",
                                         simm12shift1>;
def LDSWP_UH_Disp12 : FullLoadSwapDisp12<0b0011, "ldswp.uh",
                                         simm12shift1>;
def LDSWP_W_Disp12 : FullLoadSwapDisp12<0b1000, "ldswp.w", simm12shift2>;
def LD_SB_Disp16 : FullLoadDisp16<0b10010, "ld.sb">;
def LD_SB_IndexShift : FullLoadIndexShift<0b0000011000, "ld.sb">;
def LD_SB_AL_Disp9 : FullLoadCondDisp9Sub<0b1111, 0b011, "ld.sbal">;
def LD_SB_EQ_Disp9 : FullLoadCondDisp9Sub<0b0000, 0b011, "ld.sbeq">;
def LD_SB_NE_Disp9 : FullLoadCondDisp9Sub<0b0001, 0b011, "ld.sbne">;
def LD_SB_CC_Disp9 : FullLoadCondDisp9Sub<0b0010, 0b011, "ld.sbcc">;
def LD_SB_HS_Disp9 : FullLoadCondDisp9Sub<0b0010, 0b011, "ld.sbhs">;
def LD_SB_CS_Disp9 : FullLoadCondDisp9Sub<0b0011, 0b011, "ld.sbcs">;
def LD_SB_LO_Disp9 : FullLoadCondDisp9Sub<0b0011, 0b011, "ld.sblo">;
def LD_SB_GE_Disp9 : FullLoadCondDisp9Sub<0b0100, 0b011, "ld.sbge">;
def LD_SB_LT_Disp9 : FullLoadCondDisp9Sub<0b0101, 0b011, "ld.sblt">;
def LD_SB_MI_Disp9 : FullLoadCondDisp9Sub<0b0110, 0b011, "ld.sbmi">;
def LD_SB_PL_Disp9 : FullLoadCondDisp9Sub<0b0111, 0b011, "ld.sbpl">;
def LD_SB_LS_Disp9 : FullLoadCondDisp9Sub<0b1000, 0b011, "ld.sbls">;
def LD_SB_GT_Disp9 : FullLoadCondDisp9Sub<0b1001, 0b011, "ld.sbgt">;
def LD_SB_LE_Disp9 : FullLoadCondDisp9Sub<0b1010, 0b011, "ld.sble">;
def LD_SB_HI_Disp9 : FullLoadCondDisp9Sub<0b1011, 0b011, "ld.sbhi">;
def LD_SB_VS_Disp9 : FullLoadCondDisp9Sub<0b1100, 0b011, "ld.sbvs">;
def LD_SB_VC_Disp9 : FullLoadCondDisp9Sub<0b1101, 0b011, "ld.sbvc">;
def LD_SB_QS_Disp9 : FullLoadCondDisp9Sub<0b1110, 0b011, "ld.sbqs">;
def LD_UB_PostInc : CompactLoadPostInc<0b10011, "ld.ub">;
def LD_UB_PreDec : CompactLoadPreDec<0b10111, "ld.ub">;
def LD_UB_Disp3 : CompactLoadDisp3<"ld.ub">;
def LD_UB_Disp16 : FullLoadDisp16<0b10011, "ld.ub">;
def LD_UB_IndexShift : FullLoadIndexShift<0b0000011100, "ld.ub">;
def LD_UB_AL_Disp9 : FullLoadCondDisp9<0b1111, "ld.ubal">;
def LD_UB_EQ_Disp9 : FullLoadCondDisp9<0b0000, "ld.ubeq">;
def LD_UB_NE_Disp9 : FullLoadCondDisp9<0b0001, "ld.ubne">;
def LD_UB_CC_Disp9 : FullLoadCondDisp9<0b0010, "ld.ubcc">;
def LD_UB_HS_Disp9 : FullLoadCondDisp9<0b0010, "ld.ubhs">;
def LD_UB_CS_Disp9 : FullLoadCondDisp9<0b0011, "ld.ubcs">;
def LD_UB_LO_Disp9 : FullLoadCondDisp9<0b0011, "ld.ublo">;
def LD_UB_GE_Disp9 : FullLoadCondDisp9<0b0100, "ld.ubge">;
def LD_UB_LT_Disp9 : FullLoadCondDisp9<0b0101, "ld.ublt">;
def LD_UB_MI_Disp9 : FullLoadCondDisp9<0b0110, "ld.ubmi">;
def LD_UB_PL_Disp9 : FullLoadCondDisp9<0b0111, "ld.ubpl">;
def LD_UB_LS_Disp9 : FullLoadCondDisp9<0b1000, "ld.ubls">;
def LD_UB_GT_Disp9 : FullLoadCondDisp9<0b1001, "ld.ubgt">;
def LD_UB_LE_Disp9 : FullLoadCondDisp9<0b1010, "ld.uble">;
def LD_UB_HI_Disp9 : FullLoadCondDisp9<0b1011, "ld.ubhi">;
def LD_UB_VS_Disp9 : FullLoadCondDisp9<0b1100, "ld.ubvs">;
def LD_UB_VC_Disp9 : FullLoadCondDisp9<0b1101, "ld.ubvc">;
def LD_UB_QS_Disp9 : FullLoadCondDisp9<0b1110, "ld.ubqs">;
def LD_SH_PostInc : CompactLoadPostInc<0b10001, "ld.sh">;
def LD_SH_PreDec : CompactLoadPreDec<0b10101, "ld.sh">;
def LD_SH_Disp3 : CompactLoadDisp3Shift1<"ld.sh">;
def LD_SH_Disp16 : FullLoadDisp16<0b10000, "ld.sh">;
def LD_SH_IndexShift : FullLoadIndexShift<0b0000010000, "ld.sh">;
def LD_SH_AL_Disp9 : FullLoadCondDisp9Shift1<0b1111, "ld.shal">;
def LD_SH_EQ_Disp9 : FullLoadCondDisp9Shift1<0b0000, "ld.sheq">;
def LD_SH_NE_Disp9 : FullLoadCondDisp9Shift1<0b0001, "ld.shne">;
def LD_SH_CC_Disp9 : FullLoadCondDisp9Shift1<0b0010, "ld.shcc">;
def LD_SH_HS_Disp9 : FullLoadCondDisp9Shift1<0b0010, "ld.shhs">;
def LD_SH_CS_Disp9 : FullLoadCondDisp9Shift1<0b0011, "ld.shcs">;
def LD_SH_LO_Disp9 : FullLoadCondDisp9Shift1<0b0011, "ld.shlo">;
def LD_SH_GE_Disp9 : FullLoadCondDisp9Shift1<0b0100, "ld.shge">;
def LD_SH_LT_Disp9 : FullLoadCondDisp9Shift1<0b0101, "ld.shlt">;
def LD_SH_MI_Disp9 : FullLoadCondDisp9Shift1<0b0110, "ld.shmi">;
def LD_SH_PL_Disp9 : FullLoadCondDisp9Shift1<0b0111, "ld.shpl">;
def LD_SH_LS_Disp9 : FullLoadCondDisp9Shift1<0b1000, "ld.shls">;
def LD_SH_GT_Disp9 : FullLoadCondDisp9Shift1<0b1001, "ld.shgt">;
def LD_SH_LE_Disp9 : FullLoadCondDisp9Shift1<0b1010, "ld.shle">;
def LD_SH_HI_Disp9 : FullLoadCondDisp9Shift1<0b1011, "ld.shhi">;
def LD_SH_VS_Disp9 : FullLoadCondDisp9Shift1<0b1100, "ld.shvs">;
def LD_SH_VC_Disp9 : FullLoadCondDisp9Shift1<0b1101, "ld.shvc">;
def LD_SH_QS_Disp9 : FullLoadCondDisp9Shift1<0b1110, "ld.shqs">;
def LD_UH_PostInc : CompactLoadPostInc<0b10010, "ld.uh">;
def LD_UH_PreDec : CompactLoadPreDec<0b10110, "ld.uh">;
def LD_UH_Disp3 : CompactLoadDisp3Shift1Sub<0b01, "ld.uh">;
def LD_UH_Disp16 : FullLoadDisp16<0b10001, "ld.uh">;
def LD_UH_IndexShift : FullLoadIndexShift<0b0000010100, "ld.uh">;
def LD_UH_AL_Disp9 : FullLoadCondDisp9Shift1Sub<0b1111, 0b010, "ld.uhal">;
def LD_UH_EQ_Disp9 : FullLoadCondDisp9Shift1Sub<0b0000, 0b010, "ld.uheq">;
def LD_UH_NE_Disp9 : FullLoadCondDisp9Shift1Sub<0b0001, 0b010, "ld.uhne">;
def LD_UH_CC_Disp9 : FullLoadCondDisp9Shift1Sub<0b0010, 0b010, "ld.uhcc">;
def LD_UH_HS_Disp9 : FullLoadCondDisp9Shift1Sub<0b0010, 0b010, "ld.uhhs">;
def LD_UH_CS_Disp9 : FullLoadCondDisp9Shift1Sub<0b0011, 0b010, "ld.uhcs">;
def LD_UH_LO_Disp9 : FullLoadCondDisp9Shift1Sub<0b0011, 0b010, "ld.uhlo">;
def LD_UH_GE_Disp9 : FullLoadCondDisp9Shift1Sub<0b0100, 0b010, "ld.uhge">;
def LD_UH_LT_Disp9 : FullLoadCondDisp9Shift1Sub<0b0101, 0b010, "ld.uhlt">;
def LD_UH_MI_Disp9 : FullLoadCondDisp9Shift1Sub<0b0110, 0b010, "ld.uhmi">;
def LD_UH_PL_Disp9 : FullLoadCondDisp9Shift1Sub<0b0111, 0b010, "ld.uhpl">;
def LD_UH_LS_Disp9 : FullLoadCondDisp9Shift1Sub<0b1000, 0b010, "ld.uhls">;
def LD_UH_GT_Disp9 : FullLoadCondDisp9Shift1Sub<0b1001, 0b010, "ld.uhgt">;
def LD_UH_LE_Disp9 : FullLoadCondDisp9Shift1Sub<0b1010, 0b010, "ld.uhle">;
def LD_UH_HI_Disp9 : FullLoadCondDisp9Shift1Sub<0b1011, 0b010, "ld.uhhi">;
def LD_UH_VS_Disp9 : FullLoadCondDisp9Shift1Sub<0b1100, 0b010, "ld.uhvs">;
def LD_UH_VC_Disp9 : FullLoadCondDisp9Shift1Sub<0b1101, 0b010, "ld.uhvc">;
def LD_UH_QS_Disp9 : FullLoadCondDisp9Shift1Sub<0b1110, 0b010, "ld.uhqs">;

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

class FullRegRegLow<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rs),
                  Mnemonic#"\t$Rd, $Rs"> {
  bits<4> Rd;
  bits<4> Rs;

  let Inst{31-20} = 0b111000000000;
  let Inst{19-16} = Rs;
  let Inst{15-4} = Op;
  let Inst{3-0} = Rd;
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
  let Defs = [SREG];

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

class FullRegRegRegDiv<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs GPRDivPairOp:$RdPair), (ins GPR:$Rx, GPR:$Ry),
                  Mnemonic#"\t$RdPair, $Rx, $Ry"> {
  let Defs = [SREG];

  bits<4> RdPair;
  bits<4> Rx;
  bits<4> Ry;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rx;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Ry;
  let Inst{15-4} = Op;
  let Inst{3-0} = RdPair;
}

class FullRegRegRegDivQuot<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs GPRDivLo:$Rd), (ins GPR:$Rx, GPR:$Ry),
                  Mnemonic#"\t$Rd, $Rx, $Ry"> {
  let Defs = [SREG];

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

class FullFPURegRegReg<bits<4> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rx, GPR:$Ry),
                  Mnemonic#"\t$Rd, $Rx, $Ry"> {
  bits<4> Rd;
  bits<4> Rx;
  bits<4> Ry;

  let Inst{31-20} = 0b111001011010;
  let Inst{19-16} = Op;
  let Inst{15-12} = 0;
  let Inst{11-8} = Rd;
  let Inst{7-4} = Rx;
  let Inst{3-0} = Ry;
}

class FullFPURegReg<bits<4> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Ry),
                  Mnemonic#"\t$Rd, $Ry"> {
  bits<4> Rd;
  bits<4> Ry;

  let Inst{31-20} = 0b111001011010;
  let Inst{19-16} = Op;
  let Inst{15-12} = 0;
  let Inst{11-8} = Rd;
  let Inst{7-4} = 0;
  let Inst{3-0} = Ry;
}

class FullFPURegRegNoOut<bits<4> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Rx, GPR:$Ry),
                  Mnemonic#"\t$Rx, $Ry"> {
  bits<4> Rx;
  bits<4> Ry;

  let Inst{31-20} = 0b111001011010;
  let Inst{19-16} = Op;
  let Inst{15-8} = 0;
  let Inst{7-4} = Rx;
  let Inst{3-0} = Ry;
}

class FullFPURegNoOut<bits<4> Op, string Mnemonic>
    : AVR32Inst32<(outs), (ins GPR:$Ry), Mnemonic#"\t$Ry"> {
  bits<4> Ry;

  let Inst{31-20} = 0b111001011010;
  let Inst{19-16} = Op;
  let Inst{15-4} = 0;
  let Inst{3-0} = Ry;
}

class FullFPURegRegRegReg<bits<12> Prefix, bit Neg, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Ra, GPR:$Rx, GPR:$Ry),
                  Mnemonic#"\t$Rd, $Ra, $Rx, $Ry"> {
  bits<4> Rd;
  bits<4> Ra;
  bits<4> Rx;
  bits<4> Ry;

  let Inst{31-20} = Prefix;
  let Inst{19-16} = Ra;
  let Inst{15-13} = 0;
  let Inst{12} = Neg;
  let Inst{11-8} = Rd;
  let Inst{7-4} = Rx;
  let Inst{3-0} = Ry;
}

class FullRegRegHalfRegHalf<bits<10> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rx, halfpart:$XPart, GPR:$Ry,
                                       halfpart:$YPart),
                  Mnemonic#"\t$Rd, ${Rx}:$XPart, ${Ry}:$YPart"> {
  bits<4> Rd;
  bits<4> Rx;
  bits<1> XPart;
  bits<4> Ry;
  bits<1> YPart;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rx;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Ry;
  let Inst{15-6} = Op;
  let Inst{5} = XPart;
  let Inst{4} = YPart;
  let Inst{3-0} = Rd;
}

class FullRegRegHalf<bits<11> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rs, halfpart:$Part),
                  Mnemonic#"\t$Rd, ${Rs}:$Part"> {
  bits<4> Rd;
  bits<4> Rs;
  bits<1> Part;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rs;
  let Inst{24-16} = 0;
  let Inst{15-5} = Op;
  let Inst{4} = Part;
  let Inst{3-0} = Rd;
}

class FullRegRegRegHalf<bits<11> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rx, GPR:$Ry, halfpart:$YPart),
                  Mnemonic#"\t$Rd, $Rx, ${Ry}:$YPart"> {
  bits<4> Rd;
  bits<4> Rx;
  bits<4> Ry;
  bits<1> YPart;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rx;
  let Inst{24-20} = 0b00000;
  let Inst{19-16} = Ry;
  let Inst{15-5} = Op;
  let Inst{4} = YPart;
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

class FullRegRegUImm3Shift<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rs, uimm3:$Sa),
                  Mnemonic#"\t$Rd, $Rs, $Sa"> {
  bits<4> Rd;
  bits<4> Rs;
  bits<3> Sa;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rs;
  let Inst{24-19} = 0;
  let Inst{18-16} = Sa;
  let Inst{15-4} = Op;
  let Inst{3-0} = Rd;
}

class FullRegRegUImm4Shift<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rs, uimm4:$Sa),
                  Mnemonic#"\t$Rd, $Rs, $Sa"> {
  bits<4> Rd;
  bits<4> Rs;
  bits<4> Sa;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rs;
  let Inst{24-20} = 0;
  let Inst{19-16} = Sa;
  let Inst{15-4} = Op;
  let Inst{3-0} = Rd;
}

class FullRegRegBitField<bits<6> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rs, uimm5:$Bp, uimm5:$Width),
                  Mnemonic#"\t$Rd, $Rs, $Bp, $Width"> {
  bits<4> Rd;
  bits<4> Rs;
  bits<5> Bp;
  bits<5> Width;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rd;
  let Inst{24-20} = 0b11101;
  let Inst{19-16} = Rs;
  let Inst{15-10} = Op;
  let Inst{9-5} = Bp;
  let Inst{4-0} = Width;
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
  let Defs = [SREG];

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
  let Defs = [SREG];

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
  let Defs = [SREG];

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
  let Defs = [SREG];

  bits<4> Rd;
  bits<16> Imm;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rd;
  let Inst{15-0} = Imm;
}

class FullRegUImm16CG<bits<12> Op, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Old, uimm16:$Imm),
                  Mnemonic#"\t$Rd, $Imm"> {
  let Defs = [SREG];

  bits<4> Rd;
  bits<16> Imm;

  let Inst{31-20} = Op;
  let Inst{19-16} = Rd;
  let Inst{15-0} = Imm;
}

class FullRegUImm16Suffix<bits<12> Op, string Mnemonic, string Suffix>
    : AVR32Inst32<(outs GPR:$Rd), (ins uimm16:$Imm),
                  Mnemonic#"\t$Rd, $Imm"#Suffix> {
  let Defs = [SREG];

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

class MovRRCG<bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Old, GPR:$Rs),
                  Mnemonic#"\t$Rd, $Rs"> {
  let Uses = [SREG];
  let Constraints = "$Old = $Rd";

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

class MovCondRICG<bits<4> Cond, string Mnemonic>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Old, simm8:$Imm),
                  Mnemonic#"\t$Rd, $Imm"> {
  let Uses = [SREG];
  let Constraints = "$Old = $Rd";

  bits<4> Rd;
  bits<8> Imm;

  let Inst{31-20} = 0b111110011011;
  let Inst{19-16} = Rd;
  let Inst{15-12} = 0;
  let Inst{11-8} = Cond;
  let Inst{7-0} = Imm;
}

class SubCondRI<bits<4> Cond, string Mnemonic>
    : CondRegImm8<0b111101111011, Cond, Mnemonic>;

class SubfCondRI<bits<4> Cond, string Mnemonic>
    : CondRegImm8<0b111101011011, Cond, Mnemonic>;

class CondRRR<bits<4> Cond, bits<4> Op, string Mnemonic,
              list<dag> Patterns = []>
    : AVR32Inst32<(outs GPR:$Rd), (ins GPR:$Rx, GPR:$Ry),
                  Mnemonic#"\t$Rd, $Rx, $Ry", Patterns> {
  let Defs = [SREG];

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

let Uses = [SREG] in
def ADCrrr : FullRegRegReg<0b000000000100, "adc"> {
  let Pattern = [(set GPR:$Rd, (adde GPR:$Rx, GPR:$Ry))];
}
def ABSr : CompactReg<0b010111000100, "abs">;
def ACRr : CompactReg<0b010111000000, "acr">;
def ADDABSrrr : FullRegRegReg<0b000011100100, "addabs">;
def ADDHH_W : FullRegRegHalfRegHalf<0b0000111000, "addhh.w">;
def ADDALrrr : CondRRR<0b1111, 0b0000, "add",
                       [(set GPR:$Rd, (add GPR:$Rx, GPR:$Ry))]>;
let isCodeGenOnly = 1 in
def ADDCrrr : CondRRR<0b1111, 0b0000, "add",
                      [(set GPR:$Rd, (addc GPR:$Rx, GPR:$Ry))]>;
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
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in
def ADDrrCG : CompactRegRegCG<0b00000, "add">;
def ANDHri : FullRegUImm16<0b111001000001, "andh">;
def ANDHri_COH : FullRegUImm16Suffix<0b111001100001, "andh", ", coh">;
def ANDLri : FullRegUImm16<0b111000000001, "andl">;
def ANDLri_COH : FullRegUImm16Suffix<0b111000100001, "andl", ", coh">;
let isCodeGenOnly = 1, Constraints = "$Old = $Rd" in {
def ANDHcg : FullRegUImm16CG<0b111001000001, "andh">;
def ANDLcg : FullRegUImm16CG<0b111000000001, "andl">;
}
def ANDALrrr : CondRRR<0b1111, 0b0010, "and",
                       [(set GPR:$Rd, (and GPR:$Rx, GPR:$Ry))]>;
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
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in
def ANDrrCG : CompactRegRegCG<0b00110, "and">;
def ASRri : CompactRegUImm5Shift<0b010, "asr">;
def ASRrrr : FullRegRegReg<0b000010000100, "asr">;
def BFEXTS : FullRegRegBitField<0b101100, "bfexts">;
def BFEXTU : FullRegRegBitField<0b110000, "bfextu">;
def BFINS : FullRegRegBitField<0b110100, "bfins">;
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in
def BFINScg : AVR32Inst32<(outs GPR:$Rd),
                          (ins GPR:$Old, GPR:$Rs, uimm5:$Bp, uimm5:$Width),
                          "bfins\t$Rd, $Rs, $Bp, $Width"> {
  bits<4> Rd;
  bits<4> Rs;
  bits<5> Bp;
  bits<5> Width;

  let Inst{31-29} = 0b111;
  let Inst{28-25} = Rd;
  let Inst{24-20} = 0b11101;
  let Inst{19-16} = Rs;
  let Inst{15-10} = 0b110100;
  let Inst{9-5} = Bp;
  let Inst{4-0} = Width;
}
def BLDri : FullRegUImm5NoOut<0b111011011011, "bld">;
def BREVr : CompactReg<0b010111001001, "brev">;
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in
def BREVcg : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Old),
                       "brev\t$Rd"> {
  bits<4> Rd;

  let Inst{15-4} = 0b010111001001;
  let Inst{3-0} = Rd;
}
def BREQc : CompactBranch<0b000, "breq">;
def BRNEc : CompactBranch<0b001, "brne">;
def BRCCc : CompactBranch<0b010, "brcc">;
def BRHSc : CompactBranch<0b010, "brhs">;
def BRCSc : CompactBranch<0b011, "brcs">;
def BRLOc : CompactBranch<0b011, "brlo">;
def BRGEc : CompactBranch<0b100, "brge">;
def BRLTc : CompactBranch<0b101, "brlt">;
def BRMIc : CompactBranch<0b110, "brmi">;
def BRPLc : CompactBranch<0b111, "brpl">;
def BRALE : FullBranch<0b1111, "bral">;
def BREQe : FullBranch<0b0000, "breq">;
def BRNEe : FullBranch<0b0001, "brne">;
def BRCCe : FullBranch<0b0010, "brcc">;
def BRHSe : FullBranch<0b0010, "brhs">;
def BRCSe : FullBranch<0b0011, "brcs">;
def BRLOe : FullBranch<0b0011, "brlo">;
def BRGEe : FullBranch<0b0100, "brge">;
def BRLTe : FullBranch<0b0101, "brlt">;
def BRMIe : FullBranch<0b0110, "brmi">;
def BRPLe : FullBranch<0b0111, "brpl">;
def BRLSe : FullBranch<0b1000, "brls">;
def BRGTe : FullBranch<0b1001, "brgt">;
def BRLEe : FullBranch<0b1010, "brle">;
def BRHIe : FullBranch<0b1011, "brhi">;
def BRVSe : FullBranch<0b1100, "brvs">;
def BRVCe : FullBranch<0b1101, "brvc">;
def BRQSe : FullBranch<0b1110, "brqs">;

let isBranch = 1, isTerminator = 1, isBarrier = 1 in
def BRALbb : FullBranchCG<0b1111, "bral">;
let isBranch = 1, isTerminator = 1, Uses = [SREG] in {
def BREQbb : FullBranchCG<0b0000, "breq">;
def BRNEbb : FullBranchCG<0b0001, "brne">;
def BRCCbb : FullBranchCG<0b0010, "brcc">;
def BRCSbb : FullBranchCG<0b0011, "brcs">;
def BRGEbb : FullBranchCG<0b0100, "brge">;
def BRLTbb : FullBranchCG<0b0101, "brlt">;
def BRLSbb : FullBranchCG<0b1000, "brls">;
def BRGTbb : FullBranchCG<0b1001, "brgt">;
def BRLEbb : FullBranchCG<0b1010, "brle">;
def BRHIbb : FullBranchCG<0b1011, "brhi">;
}

let isBranch = 1, isTerminator = 1, isBarrier = 1 in
def RJMPbb : CompactRjmpCG<"rjmp">;
let isBranch = 1, isTerminator = 1, Uses = [SREG] in {
def BREQcbb : CompactBranchCG<0b000, "breq">;
def BRNEcbb : CompactBranchCG<0b001, "brne">;
def BRCCcbb : CompactBranchCG<0b010, "brcc">;
def BRCScbb : CompactBranchCG<0b011, "brcs">;
def BRGEcbb : CompactBranchCG<0b100, "brge">;
def BRLTcbb : CompactBranchCG<0b101, "brlt">;
}

def BSTri : FullRegUImm5NoOut<0b111011111011, "bst">;
def BREAKPOINT : AVR32Inst<0b1101011001110011, (outs), (ins), "breakpoint">;
def CBRri : CompactRegUImm5NoOut<0b1110, "cbr">;
def CACHE : FullMemDisp11UImm5<0b111101000001, "cache">;
def CASTS_Br : CompactReg<0b010111000110, "casts.b">;
def CASTS_Hr : CompactReg<0b010111001000, "casts.h">;
def CASTU_Br : CompactReg<0b010111000101, "castu.b">;
def CASTU_Hr : CompactReg<0b010111000111, "castu.h">;
let isCodeGenOnly = 1, Constraints = "$Old = $Rd" in {
def CASTS_Bcg : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Old),
                          "casts.b\t$Rd"> {
  bits<4> Rd;

  let Inst{15-4} = 0b010111000110;
  let Inst{3-0} = Rd;
}
def CASTS_Hcg : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Old),
                          "casts.h\t$Rd"> {
  bits<4> Rd;

  let Inst{15-4} = 0b010111001000;
  let Inst{3-0} = Rd;
}
def CASTU_Bcg : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Old),
                          "castu.b\t$Rd"> {
  bits<4> Rd;

  let Inst{15-4} = 0b010111000101;
  let Inst{3-0} = Rd;
}
def CASTU_Hcg : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Old),
                          "castu.h\t$Rd"> {
  bits<4> Rd;

  let Inst{15-4} = 0b010111000111;
  let Inst{3-0} = Rd;
}
}
def CLZrr : FullRegReg<0b0001001000000000, "clz">;
def COMr : CompactReg<0b010111001101, "com">;
def COP : CoprocessorOp<"cop">;
def PICOVMUL0 : PicoCoprocessorOp<0x00, "out0", "picovmul">;
def PICOVMUL1 : PicoCoprocessorOp<0x01, "out1", "picovmul">;
def PICOVMUL2 : PicoCoprocessorOp<0x02, "out2", "picovmul">;
def PICOVMUL3 : PicoCoprocessorOp<0x03, "out3", "picovmul">;
def PICOVMAC0 : PicoCoprocessorOp<0x04, "out0", "picovmac">;
def PICOVMAC1 : PicoCoprocessorOp<0x05, "out1", "picovmac">;
def PICOVMAC2 : PicoCoprocessorOp<0x06, "out2", "picovmac">;
def PICOVMAC3 : PicoCoprocessorOp<0x07, "out3", "picovmac">;
def PICOSVMUL0 : PicoCoprocessorOp<0x08, "out0", "picosvmul">;
def PICOSVMUL1 : PicoCoprocessorOp<0x09, "out1", "picosvmul">;
def PICOSVMUL2 : PicoCoprocessorOp<0x0a, "out2", "picosvmul">;
def PICOSVMUL3 : PicoCoprocessorOp<0x0b, "out3", "picosvmul">;
def PICOSVMAC0 : PicoCoprocessorOp<0x0c, "out0", "picosvmac">;
def PICOSVMAC1 : PicoCoprocessorOp<0x0d, "out1", "picosvmac">;
def PICOSVMAC2 : PicoCoprocessorOp<0x0e, "out2", "picosvmac">;
def PICOSVMAC3 : PicoCoprocessorOp<0x0f, "out3", "picosvmac">;
def PICOMV_D_FromReg : PicoMoveRegToCopRegD<"picomv.d">;
def PICOMV_D_ToReg : PicoMoveCopRegToRegD<"picomv.d">;
def PICOMV_W_FromReg : PicoMoveRegToCopRegW<"picomv.w">;
def PICOMV_W_ToReg : PicoMoveCopRegToRegW<"picomv.w">;
let hasSideEffects = 1 in {
let Uses = [SREG], Defs = [SREG] in {
def CPCr : CompactRegNoOut<0b010111000010, "cpc">;
def CPCrr : FullRegRegNoOut<0b0001001100000000, "cpc">;
}
def CP_Brr : FullRegRegNoOut<0b0001100000000000, "cp.b">;
def CP_Hrr : FullRegRegNoOut<0b0001100100000000, "cp.h">;
def CPri21 : FullRegImmNoOut<0b0010, "cp">;
def CPri6 : CompactRegImmNoOut<0b010110, "cp">;
def CPrr : CompactRegRegNoOut<0b00011, "cp">;
def CP_Wri21 : FullRegImmNoOut<0b0010, "cp.w">;
def CP_Wri6 : CompactRegImmNoOut<0b010110, "cp.w">;
def CP_Wrr : CompactRegRegNoOut<0b00011, "cp.w">;
}
def CSRFCZi : CompactImm<0b1101000, "csrfcz">;
def CSRFi : CompactImm<0b1101010, "csrf">;
def DIVSrrr : FullRegRegReg<0b000011000000, "divs">;
def DIVUrrr : FullRegRegReg<0b000011010000, "divu">;
let isCodeGenOnly = 1 in {
def DIVSrrrQ : FullRegRegRegDivQuot<0b000011000000, "divs">,
               Sched<[]> {
  let Pattern = [(set GPRDivLo:$Rd, (AVR32sdiv GPR:$Rx, GPR:$Ry))];
}
def DIVUrrrQ : FullRegRegRegDivQuot<0b000011010000, "divu">,
               Sched<[]> {
  let Pattern = [(set GPRDivLo:$Rd, (AVR32udiv GPR:$Rx, GPR:$Ry))];
}
def DIVSrrrQR : FullRegRegRegDiv<0b000011000000, "divs">,
                Sched<[]> {
  let Pattern = [(set GPRDivPairOp:$RdPair,
                      (AVR32sdivrem GPR:$Rx, GPR:$Ry))];
}
def DIVUrrrQR : FullRegRegRegDiv<0b000011010000, "divu">,
                Sched<[]> {
  let Pattern = [(set GPRDivPairOp:$RdPair,
                      (AVR32udivrem GPR:$Rx, GPR:$Ry))];
}
}
def EORHri : FullRegUImm16<0b111011100001, "eorh">;
def EORLri : FullRegUImm16<0b111011000001, "eorl">;
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in {
def EORHcg : FullRegUImm16CG<0b111011100001, "eorh">;
def EORLcg : FullRegUImm16CG<0b111011000001, "eorl">;
}
def EORALrrr : CondRRR<0b1111, 0b0100, "eor",
                       [(set GPR:$Rd, (xor GPR:$Rx, GPR:$Ry))]>;
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
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in
def EORrrCG : CompactRegRegCG<0b00101, "eor">;
def FRS : AVR32Inst<0b1101011101000011, (outs), (ins), "frs">;
def FADD_Srrr : FullFPURegRegReg<0b0000, "fadd.s">;
def FCASTRS_SWrr : FullFPURegReg<0b1011, "fcastrs.sw">;
def FCASTRS_UWrr : FullFPURegReg<0b1001, "fcastrs.uw">;
def FCASTSW_Srr : FullFPURegReg<0b0110, "fcastsw.s">;
def FCASTUW_Srr : FullFPURegReg<0b0100, "fcastuw.s">;
def FCHK_Sr : FullFPURegNoOut<0b1101, "fchk.s">;
def FCMP_Srr : FullFPURegRegNoOut<0b1100, "fcmp.s">;
def FMAC_Srrrr : FullFPURegRegRegReg<0b111000011010, 0, "fmac.s">;
def FNMUL_Srrr : FullFPURegRegReg<0b0011, "fnmul.s">;
def FNMAC_Srrrr : FullFPURegRegRegReg<0b111000011010, 1, "fnmac.s">;
def FNMSC_Srrrr : FullFPURegRegRegReg<0b111000111010, 1, "fnmsc.s">;
def FMUL_Srrr : FullFPURegRegReg<0b0010, "fmul.s">;
def FMSC_Srrrr : FullFPURegRegRegReg<0b111000111010, 0, "fmsc.s">;
def FSUB_Srrr : FullFPURegRegReg<0b0001, "fsub.s">;
def FRCPA_Srr : FullFPURegReg<0b1110, "frcpa.s">;
def FRSQRTA_Srr : FullFPURegReg<0b1111, "frsqrta.s">;
let isCall = 1, Defs = [LR] in
def ICALLr : CompactRegNoOut<0b010111010001, "icall">;
def LDM : FullRegList<0b000, 0, "ldm\t$Rp, $Mask">;
def LDM_UPD : FullRegList<0b000, 1, "ldm\t${Rp}++, $Mask">;
def LDMTS : FullRegList<0b001, 0, "ldmts\t$Rp, $Mask">;
def LDMTS_UPD : FullRegList<0b001, 1, "ldmts\t${Rp}++, $Mask">;
def LDCM_D : CopLoadMultipleLow<0b0100, 0, coprocessorreglistd,
                                "ldcm.d\t$CP, $Rp, $Mask">;
def LDCM_D_UPD : CopLoadMultipleLow<0b0100, 1, coprocessorreglistd,
                                    "ldcm.d\t$CP, ${Rp}++, $Mask">;
def LDCM_W_High : CopLoadMultipleHigh<0b0001, 0,
                                      "ldcm.w\t$CP, $Rp, $Mask">;
def LDCM_W_High_UPD : CopLoadMultipleHigh<0b0001, 1,
                                          "ldcm.w\t$CP, ${Rp}++, $Mask">;
def LDCM_W_Low : CopLoadMultipleLow<0b0000, 0, coprocessorreglistlow,
                                    "ldcm.w\t$CP, $Rp, $Mask">;
def LDCM_W_Low_UPD : CopLoadMultipleLow<0b0000, 1, coprocessorreglistlow,
                                        "ldcm.w\t$CP, ${Rp}++, $Mask">;
def PICOLDM_D : PicoLoadMultipleLow<0b0100, 0, picoreglistd,
                                    "picoldm.d\t$Rp, $Mask">;
def PICOLDM_D_UPD : PicoLoadMultipleLow<0b0100, 1, picoreglistd,
                                        "picoldm.d\t${Rp}++, $Mask">;
def PICOLDM_W_High : PicoLoadMultipleHigh<0b0001, 0,
                                          "picoldm.w\t$Rp, $Mask">;
def PICOLDM_W_High_UPD : PicoLoadMultipleHigh<0b0001, 1,
                                              "picoldm.w\t${Rp}++, $Mask">;
def PICOLDM_W_Low : PicoLoadMultipleLow<0b0000, 0, picoreglistlow,
                                        "picoldm.w\t$Rp, $Mask">;
def PICOLDM_W_Low_UPD : PicoLoadMultipleLow<0b0000, 1, picoreglistlow,
                                            "picoldm.w\t${Rp}++, $Mask">;
def LSLri : CompactRegUImm5Shift<0b011, "lsl">;
def LSLrrr : FullRegRegReg<0b000010010100, "lsl">;
def LSRri : CompactRegUImm5Shift<0b100, "lsr">;
def LSRrrr : FullRegRegReg<0b000010100100, "lsr">;
def MAXrrr : FullRegRegReg<0b000011000100, "max">;
let isCall = 1, Defs = [LR], Size = 4 in
def CALLp : AVR32Pseudo<(outs), (ins ldaexpr:$Disp), "call\t$Disp">;
def MCALL : FullMemDisp16Shift2<0b111100000001, "mcall">;
def MCALLcp : FullMemPCRel16Shift2<0b111100000001, "mcall">;
def MINrrr : FullRegRegReg<0b000011010100, "min">;
def MFDRi : FullRegDebugReg<0b111001011011, "mfdr">;
def MFSRi : FullRegSysReg<0b111000011011, "mfsr">;
let isCodeGenOnly = 1 in
def MOVALri : MovCondRI<0b1111, "mov">;
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
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in {
def MOVEQriCG : MovCondRICG<0b0000, "moveq">;
def MOVNEriCG : MovCondRICG<0b0001, "movne">;
def MOVCCriCG : MovCondRICG<0b0010, "movcc">;
def MOVCSriCG : MovCondRICG<0b0011, "movcs">;
def MOVGEriCG : MovCondRICG<0b0100, "movge">;
def MOVLTriCG : MovCondRICG<0b0101, "movlt">;
def MOVLSriCG : MovCondRICG<0b1000, "movls">;
def MOVGTriCG : MovCondRICG<0b1001, "movgt">;
def MOVLEriCG : MovCondRICG<0b1010, "movle">;
def MOVHIriCG : MovCondRICG<0b1011, "movhi">;
}
let isCodeGenOnly = 1 in
def MOVALrr : MovRR<0b1111, "mov">;
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
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in {
def MOVEQrrCG : MovRRCG<0b0000, "moveq">;
def MOVNErrCG : MovRRCG<0b0001, "movne">;
def MOVCCrrCG : MovRRCG<0b0010, "movcc">;
def MOVCSrrCG : MovRRCG<0b0011, "movcs">;
def MOVGErrCG : MovRRCG<0b0100, "movge">;
def MOVLTrrCG : MovRRCG<0b0101, "movlt">;
def MOVLSrrCG : MovRRCG<0b1000, "movls">;
def MOVGTrrCG : MovRRCG<0b1001, "movgt">;
def MOVLErrCG : MovRRCG<0b1010, "movle">;
def MOVHIrrCG : MovRRCG<0b1011, "movhi">;
}
def MOVrr : CompactRegReg<0b01001, "mov">;
def MOVri8 : MovRI<0b0011, "mov">;
def MOVri21 : FullRegImm<0b0011, "mov"> {
  let Pattern = [(set GPR:$Rd, simm21:$Imm)];
}
def MOVHri : FullRegUImm16<0b111111000001, "movh">;
def MVCRd : MoveCopRegToRegD<"mvcr.d">;
def MVCRw : MoveCopRegToRegW<"mvcr.w">;
def MVRCd : MoveRegToCopRegD<"mvrc.d">;
def MVRCw : MoveRegToCopRegW<"mvrc.w">;
def MACrrr : FullRegRegReg<0b000000110100, "mac">;
def MACHH_D : FullRegRegHalfRegHalf<0b0000010110, "machh.d">;
def MACHH_W : FullRegRegHalfRegHalf<0b0000010010, "machh.w">;
def MACS_Drrr : FullRegRegReg<0b000001010100, "macs.d">;
def MACSATHH_W : FullRegRegHalfRegHalf<0b0000011010, "macsathh.w">;
def MACU_Drrr : FullRegRegReg<0b000001110100, "macu.d">;
def MACWH_D : FullRegRegRegHalf<0b00001100100, "macwh.d">;
def MEMC : FullMemBit<0b111111000001, "memc">;
def MEMS : FullMemBit<0b111100000001, "mems">;
def MEMT : FullMemBit<0b111101000001, "memt">;
def MULrr : CompactRegRegWithPrefix<0b101, 0b10011, "mul">;
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in
def MULrrCG : CompactRegRegWithPrefixCG<0b101, 0b10011, "mul">;
def MULri8 : FullRegRegImm<0b00010000, "mul">;
def MULrrr : FullRegRegReg<0b000000100100, "mul">;
def MULHH_W : FullRegRegHalfRegHalf<0b0000011110, "mulhh.w">;
def MULNHH_W : FullRegRegHalfRegHalf<0b0000000110, "mulnhh.w">;
def MULNWH_D : FullRegRegRegHalf<0b00000010100, "mulnwh.d">;
def MULSATHH_H : FullRegRegHalfRegHalf<0b0000100010, "mulsathh.h">;
def MULSATHH_W : FullRegRegHalfRegHalf<0b0000100110, "mulsathh.w">;
def MULSATRNDHH_H : FullRegRegHalfRegHalf<0b0000101010, "mulsatrndhh.h">;
def MULSATRNDWH_W : FullRegRegRegHalf<0b00001011100, "mulsatrndwh.w">;
def MULSATWH_W : FullRegRegRegHalf<0b00001110100, "mulsatwh.w">;
def MULS_Drrr : FullRegRegReg<0b000001000100, "muls.d">;
def MULU_Drrr : FullRegRegReg<0b000001100100, "mulu.d">;
let isCodeGenOnly = 1 in
def MULU_DrrrP : FullRegRegRegDiv<0b000001100100, "mulu.d">,
                 Sched<[]> {
  let Pattern = [(set GPRDivPairOp:$RdPair,
                      (AVR32umullohi GPR:$Rx, GPR:$Ry))];
}
def MULWH_D : FullRegRegRegHalf<0b00001101100, "mulwh.d">;
def MUSFRr : CompactRegNoOut<0b010111010011, "musfr">;
def MUSTRr : CompactReg<0b010111010010, "mustr">;
def MTDRi : FullDebugRegReg<0b111001111011, "mtdr">;
def MTSRi : FullSysRegReg<0b111000111011, "mtsr">;
def NEGr : CompactReg<0b010111000011, "neg">;
def ORHri : FullRegUImm16<0b111010100001, "orh">;
def ORLri : FullRegUImm16<0b111010000001, "orl">;
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in {
def ORHcg : FullRegUImm16CG<0b111010100001, "orh">;
def ORLcg : FullRegUImm16CG<0b111010000001, "orl">;
}
def ORALrrr : CondRRR<0b1111, 0b0011, "or",
                      [(set GPR:$Rd, (or GPR:$Rx, GPR:$Ry))]>;
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
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in
def ORrrCG : CompactRegRegCG<0b00100, "or">;
def PABS_SBrr : FullRegRegLow<0b001000111110, "pabs.sb">;
def PABS_SHrr : FullRegRegLow<0b001000111111, "pabs.sh">;
def PACKSH_UBrrr : FullRegRegReg<0b001001001100, "packsh.ub">;
def PACKSH_SBrrr : FullRegRegReg<0b001001001101, "packsh.sb">;
def PACKW_SHrrr : FullRegRegReg<0b001001000111, "packw.sh">;
def PADD_Brrr : FullRegRegReg<0b001000110000, "padd.b">;
def PADD_Hrrr : FullRegRegReg<0b001000000000, "padd.h">;
def PADDH_UBrrr : FullRegRegReg<0b001000110110, "paddh.ub">;
def PADDH_SHrrr : FullRegRegReg<0b001000001100, "paddh.sh">;
def PADDS_UBrrr : FullRegRegReg<0b001000110100, "padds.ub">;
def PADDS_SBrrr : FullRegRegReg<0b001000110010, "padds.sb">;
def PADDS_UHrrr : FullRegRegReg<0b001000001000, "padds.uh">;
def PADDS_SHrrr : FullRegRegReg<0b001000000100, "padds.sh">;
def PADDSUB_H : FullRegRegHalfRegHalf<0b0010000100, "paddsub.h">;
def PADDSUBH_SH : FullRegRegHalfRegHalf<0b0010001010, "paddsubh.sh">;
def PADDSUBS_UH : FullRegRegHalfRegHalf<0b0010001000, "paddsubs.uh">;
def PADDSUBS_SH : FullRegRegHalfRegHalf<0b0010000110, "paddsubs.sh">;
def PADDX_Hrrr : FullRegRegReg<0b001000000010, "paddx.h">;
def PADDXH_SHrrr : FullRegRegReg<0b001000001110, "paddxh.sh">;
def PADDXS_UHrrr : FullRegRegReg<0b001000001010, "paddxs.uh">;
def PADDXS_SHrrr : FullRegRegReg<0b001000000110, "paddxs.sh">;
def PASR_Brri : FullRegRegUImm3Shift<0b001001000001, "pasr.b">;
def PASR_Hrri : FullRegRegUImm4Shift<0b001001000100, "pasr.h">;
def PLSL_Brri : FullRegRegUImm3Shift<0b001001000010, "plsl.b">;
def PLSL_Hrri : FullRegRegUImm4Shift<0b001001000101, "plsl.h">;
def PLSR_Brri : FullRegRegUImm3Shift<0b001001000011, "plsr.b">;
def PLSR_Hrri : FullRegRegUImm4Shift<0b001001000110, "plsr.h">;
def PAVG_UBrrr : FullRegRegReg<0b001000111100, "pavg.ub">;
def PAVG_SHrrr : FullRegRegReg<0b001000111101, "pavg.sh">;
def PREF : FullMemDisp16<0b111100100001, "pref">;
def RCALLc : CompactPCRel10<0b11, "rcall">;
let isCall = 1, Defs = [LR] in
def RCALLe : FullPCRel21<"rcall">;
let isCall = 1, Defs = [LR] in
def RCALLeCG : FullPCRel21CG<"rcall">;
def : InstAlias<"rcall\t$Disp", (RCALLe PC, simm21pcrelshift1:$Disp), 0>;
def PMAX_UBrrr : FullRegRegReg<0b001000111000, "pmax.ub">;
def PMAX_SHrrr : FullRegRegReg<0b001000111001, "pmax.sh">;
def PMIN_UBrrr : FullRegRegReg<0b001000111010, "pmin.ub">;
def PMIN_SHrrr : FullRegRegReg<0b001000111011, "pmin.sh">;
def PSADrrr : FullRegRegReg<0b001001000000, "psad">;
def PSUB_Brrr : FullRegRegReg<0b001000110001, "psub.b">;
def PSUB_Hrrr : FullRegRegReg<0b001000000001, "psub.h">;
def PSUBADD_H : FullRegRegHalfRegHalf<0b0010000101, "psubadd.h">;
def PSUBADDH_SH : FullRegRegHalfRegHalf<0b0010001011, "psubaddh.sh">;
def PSUBADDS_UH : FullRegRegHalfRegHalf<0b0010001001, "psubadds.uh">;
def PSUBADDS_SH : FullRegRegHalfRegHalf<0b0010000111, "psubadds.sh">;
def PSUBH_UBrrr : FullRegRegReg<0b001000110111, "psubh.ub">;
def PSUBH_SHrrr : FullRegRegReg<0b001000001101, "psubh.sh">;
def PSUBS_UBrrr : FullRegRegReg<0b001000110101, "psubs.ub">;
def PSUBS_SBrrr : FullRegRegReg<0b001000110011, "psubs.sb">;
def PSUBS_UHrrr : FullRegRegReg<0b001000001001, "psubs.uh">;
def PSUBS_SHrrr : FullRegRegReg<0b001000000101, "psubs.sh">;
def PSUBX_Hrrr : FullRegRegReg<0b001000000011, "psubx.h">;
def PSUBXH_SHrrr : FullRegRegReg<0b001000001111, "psubxh.sh">;
def PSUBXS_UHrrr : FullRegRegReg<0b001000001011, "psubxs.uh">;
def PSUBXS_SHrrr : FullRegRegReg<0b001000000111, "psubxs.sh">;
def PUNPCKUB_H : FullRegRegHalf<0b00100100100, "punpckub.h">;
def PUNPCKSB_H : FullRegRegHalf<0b00100100101, "punpcksb.h">;
def POPM : CompactRegList8<0b0010, "popm">;
let isBarrier = 1, isCodeGenOnly = 1, isReturn = 1, isTerminator = 1 in
def POPM_RET : CompactRegList8<0b0010, "popm">;
let isBarrier = 1, isReturn = 1, isTerminator = 1 in
def POPM_RETVAL : CompactPopmRetVal;
def POPJC : AVR32Inst<0b1101011100010011, (outs), (ins), "popjc">;
def PUSHM : CompactRegList8<0b0001, "pushm">;
def PUSHJC : AVR32Inst<0b1101011100100011, (outs), (ins), "pushjc">;
def RJMP : CompactRjmp<"rjmp">;
def : InstAlias<"rjmp\t$Disp", (BRALE simm21pcrelshift1:$Disp), 0>;
let isBarrier = 1, isReturn = 1, isTerminator = 1 in {
def RETD : AVR32Inst<0b1101011000100011, (outs), (ins), "retd">;
def RETE : AVR32Inst<0b1101011000000011, (outs), (ins), "rete",
                     [(AVR32retintrflag)]>;
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
let isBarrier = 1, isCodeGenOnly = 1, isReturn = 1, isTerminator = 1,
    hasCtrlDep = 1, isBranch = 1 in
def RETR12 : AVR32Inst<0b0101111011111100, (outs), (ins), "ret\tr12",
                       [(AVR32retflag)]>;
let isBarrier = 1, isReturn = 1, isTerminator = 1 in {
def RETJ : AVR32Inst<0b1101011000110011, (outs), (ins), "retj">;
def RETS : AVR32Inst<0b1101011000010011, (outs), (ins), "rets">;
def RETSS : AVR32Inst<0b1101011101100011, (outs), (ins), "retss">;
}

let isCodeGenOnly = 1, Defs = [SP], Uses = [SP] in {
def ADJCALLSTACKDOWN : AVR32Inst<0, (outs), (ins i32imm:$amt1, i32imm:$amt2),
                                "#ADJCALLSTACKDOWN $amt1 $amt2",
                                [(AVR32callseq_start timm:$amt1,
                                                       timm:$amt2)]>;
def ADJCALLSTACKUP : AVR32Inst<0, (outs), (ins i32imm:$amt1, i32imm:$amt2),
                              "#ADJCALLSTACKUP $amt1 $amt2",
                              [(AVR32callseq_end timm:$amt1, timm:$amt2)]>;
}

def ROLr : CompactReg<0b010111001111, "rol">;
def RORr : CompactReg<0b010111010000, "ror">;
def RSUBALri : CondRegImm8<0b111110111011, 0b1111, "rsub">;
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
def SATADD_Hrrr : FullRegRegReg<0b000000101100, "satadd.h">;
def SATADD_Wrrr : FullRegRegReg<0b000000001100, "satadd.w">;
def SATRNDSri : FullRegShiftBits<0b111100111011, 0b000000, "satrnds">;
def SATRNDUri : FullRegShiftBits<0b111100111011, 0b000001, "satrndu">;
def SATSUB_Hrrr : FullRegRegReg<0b000000111100, "satsub.h">;
def SATSUB_Wrrr : FullRegRegReg<0b000000011100, "satsub.w">;
def SATSUB_Wrri16 : FullRegRegSImm16<0b01101, "satsub.w">;
def SATSri : FullRegShiftBits<0b111100011011, 0b000000, "sats">;
def SATUri : FullRegShiftBits<0b111100011011, 0b000001, "satu">;
let Uses = [SREG] in
def SBCrrr : FullRegRegReg<0b000000010100, "sbc"> {
  let Pattern = [(set GPR:$Rd, (sube GPR:$Rx, GPR:$Ry))];
}
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
def STC_D_Disp : CopStoreDispD<0b0111010, "stc.d">;
def STC_D_PostInc : CopStorePostIncD<0b001110000, "stc.d">;
def STC_D_IndexShift : CopStoreIndexShiftD<"stc.d">;
def STC_W_Disp : CopStoreDispW<0b0111010, "stc.w">;
def STC_W_PostInc : CopStorePostIncW<0b01100000, "stc.w">;
def STC_W_IndexShift : CopStoreIndexShiftW<"stc.w">;
def PICOST_D_Disp : PicoStoreDispD<0b0111010, "picost.d">;
def PICOST_D_PostInc : PicoStorePostIncD<0b001110000, "picost.d">;
def PICOST_D_IndexShift : PicoStoreIndexShiftD<"picost.d">;
def PICOST_W_Disp : PicoStoreDispW<0b0111010, "picost.w">;
def PICOST_W_PostInc : PicoStorePostIncW<0b01100000, "picost.w">;
def PICOST_W_IndexShift : PicoStoreIndexShiftW<"picost.w">;
def STC0_D_Disp : Cop0StoreDispD<0b1111010, "stc0.d">;
def STC0_W_Disp : Cop0StoreDispW<0b1011010, "stc0.w">;
def STCOND : FullStoreDisp16<0b10111, "stcond">;
def STDSP : CompactSPStoreDisp7Shift2;
def STHH_W_Disp8 : FullStoreHalfwordPairDisp8;
def STHH_W_IndexShift : FullStoreHalfwordPairIndexShift;
def STSWP_H_Disp12 : FullStoreSwapDisp12<0b1001, "stswp.h", simm12shift1>;
def STSWP_W_Disp12 : FullStoreSwapDisp12<0b1010, "stswp.w", simm12shift2>;
def STM : FullRegList<0b010, 0, "stm\t$Rp, $Mask">;
def STM_PRE : FullRegList<0b010, 1, "stm\t--$Rp, $Mask">;
def STMTS : FullRegList<0b011, 0, "stmts\t$Rp, $Mask">;
def STMTS_PRE : FullRegList<0b011, 1, "stmts\t--$Rp, $Mask">;
def STCM_D : CopStoreMultipleLow<0b0101, 0, coprocessorreglistd,
                                 "stcm.d\t$CP, $Rp, $Mask">;
def STCM_D_PRE : CopStoreMultipleLow<0b0101, 1, coprocessorreglistd,
                                     "stcm.d\t$CP, --$Rp, $Mask">;
def STCM_W_High : CopStoreMultipleHigh<0b0011, 0,
                                       "stcm.w\t$CP, $Rp, $Mask">;
def STCM_W_High_PRE : CopStoreMultipleHigh<0b0011, 1,
                                           "stcm.w\t$CP, --$Rp, $Mask">;
def STCM_W_Low : CopStoreMultipleLow<0b0010, 0, coprocessorreglistlow,
                                     "stcm.w\t$CP, $Rp, $Mask">;
def STCM_W_Low_PRE : CopStoreMultipleLow<0b0010, 1, coprocessorreglistlow,
                                         "stcm.w\t$CP, --$Rp, $Mask">;
def PICOSTM_D : PicoStoreMultipleLow<0b0101, 0, picoreglistd,
                                     "picostm.d\t$Rp, $Mask">;
def PICOSTM_D_PRE : PicoStoreMultipleLow<0b0101, 1, picoreglistd,
                                         "picostm.d\t--$Rp, $Mask">;
def PICOSTM_W_High : PicoStoreMultipleHigh<0b0011, 0,
                                           "picostm.w\t$Rp, $Mask">;
def PICOSTM_W_High_PRE : PicoStoreMultipleHigh<0b0011, 1,
                                               "picostm.w\t--$Rp, $Mask">;
def PICOSTM_W_Low : PicoStoreMultipleLow<0b0010, 0, picoreglistlow,
                                         "picostm.w\t$Rp, $Mask">;
def PICOSTM_W_Low_PRE : PicoStoreMultipleLow<0b0010, 1, picoreglistlow,
                                             "picostm.w\t--$Rp, $Mask">;
def SUBALrrr : CondRRR<0b1111, 0b0001, "sub",
                       [(set GPR:$Rd, (sub GPR:$Rx, GPR:$Ry))]>;
let isCodeGenOnly = 1 in
def SUBCrrr : CondRRR<0b1111, 0b0001, "sub",
                      [(set GPR:$Rd, (subc GPR:$Rx, GPR:$Ry))]>;
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
def SUBALri : SubCondRI<0b1111, "sub">;
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
def SUBHH_W : FullRegRegHalfRegHalf<0b0000111100, "subhh.w">;
def SUBSPri8 : CompactSPSubImm<"sub">;
def SUBri8 : CompactRegSImm8<0b0010, "sub">;
def SUBrrrs : FullRegRegRegShift<0b0000000100, "sub">;
def SUBrri16 : FullRegRegSImm16<0b01100, "sub">;
def SUBri21 : FullRegImm<0b0001, "sub">;
def SUBrr : CompactRegReg<0b00001, "sub">;
let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in
def SUBrrCG : CompactRegRegCG<0b00001, "sub">;
def SWAP_BHr : CompactReg<0b010111001100, "swap.bh">;
def SWAP_Br : CompactReg<0b010111001011, "swap.b">;
def SWAP_Hr : CompactReg<0b010111001010, "swap.h">;
let isCodeGenOnly = 1, Constraints = "$Old = $Rd" in {
def SWAP_BHcg : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Old),
                          "swap.bh\t$Rd"> {
  bits<4> Rd;

  let Inst{15-4} = 0b010111001100;
  let Inst{3-0} = Rd;
}
def SWAP_Bcg : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Old),
                         "swap.b\t$Rd"> {
  bits<4> Rd;

  let Inst{15-4} = 0b010111001011;
  let Inst{3-0} = Rd;
}
def SWAP_Hcg : AVR32Inst<0, (outs GPR:$Rd), (ins GPR:$Old),
                         "swap.h\t$Rd"> {
  bits<4> Rd;

  let Inst{15-4} = 0b010111001010;
  let Inst{3-0} = Rd;
}
}
def : Pat<(bswap GPR:$Src), (SWAP_Bcg GPR:$Src)>;
def SYNCi : FullImm8NoOut<0b1110011101100000, "sync">;
def TLBR : AVR32Inst<0b1101011001000011, (outs), (ins), "tlbr">;
def TLBS : AVR32Inst<0b1101011001010011, (outs), (ins), "tlbs">;
def TLBW : AVR32Inst<0b1101011001100011, (outs), (ins), "tlbw">;
def TNBZr : CompactRegNoOut<0b010111001110, "tnbz">;
let hasSideEffects = 1 in
def TSTrr : CompactRegRegNoOut<0b00111, "tst">;
def XCHGrrr : FullRegRegReg<0b000010110100, "xchg">;

let isCodeGenOnly = 1, Constraints = "$Rd = $Old" in {
def ASRricg : CompactRegUImm5ShiftCG<0b010, "asr">;
def LSLricg : CompactRegUImm5ShiftCG<0b011, "lsl">;
def LSRricg : CompactRegUImm5ShiftCG<0b100, "lsr">;
}

let usesCustomInserter = 1, isCodeGenOnly = 1 in
def SETCCrr : AVR32Inst<0, (outs GPR:$Rd),
                        (ins GPR:$LHS, GPR:$RHS, i32imm:$Cond),
                        "# SETCCrr",
                        [(set GPR:$Rd,
                          (AVR32setcc GPR:$LHS, GPR:$RHS, timm:$Cond))]>;

let usesCustomInserter = 1, isCodeGenOnly = 1 in
def SETCC64rr : AVR32Inst<0, (outs GPR:$Rd),
                          (ins GPR:$LHSLo, GPR:$LHSHi, GPR:$RHSLo,
                               GPR:$RHSHi, i32imm:$Cond),
                          "# SETCC64rr",
                          [(set GPR:$Rd,
                            (AVR32setcc64 GPR:$LHSLo, GPR:$LHSHi, GPR:$RHSLo,
                                            GPR:$RHSHi, timm:$Cond))]>;

let usesCustomInserter = 1, isCodeGenOnly = 1 in
def SELECTCCrr : AVR32Inst<0, (outs GPR:$Rd),
                           (ins GPR:$LHS, GPR:$RHS, GPR:$True, GPR:$False,
                                i32imm:$Cond),
                           "# SELECTCCrr",
                           [(set GPR:$Rd,
                             (AVR32selectcc GPR:$LHS, GPR:$RHS, GPR:$True,
                                            GPR:$False, timm:$Cond))]>;

let isCodeGenOnly = 1 in
def FIADDR : AVR32Inst<0, (outs GPR:$Rd), (ins i32imm:$Frame, simm16:$Disp),
                       "# FIADDR">;

def : Pat<(AVR32call (i32 tglobaladdr:$dst)), (CALLp tglobaladdr:$dst)>;
def : Pat<(AVR32call (i32 texternalsym:$dst)),
          (CALLp texternalsym:$dst)>;
def : Pat<(AVR32call GPR:$dst), (ICALLr GPR:$dst)>;

def : Pat<(br bb:$dst), (BRALbb bb:$dst)>;

def : Pat<(shl GPR:$LHS, uimm5:$RHS), (LSLricg GPR:$LHS, uimm5:$RHS)>;
def : Pat<(srl GPR:$LHS, uimm5:$RHS), (LSRricg GPR:$LHS, uimm5:$RHS)>;
def : Pat<(sra GPR:$LHS, uimm5:$RHS), (ASRricg GPR:$LHS, uimm5:$RHS)>;
def : Pat<(shl GPR:$LHS, GPR:$RHS), (LSLrrr GPR:$LHS, GPR:$RHS)>;
def : Pat<(srl GPR:$LHS, GPR:$RHS), (LSRrrr GPR:$LHS, GPR:$RHS)>;
def : Pat<(sra GPR:$LHS, GPR:$RHS), (ASRrrr GPR:$LHS, GPR:$RHS)>;
def : Pat<(and (srl GPR:$Src, uimm5:$Bp), bfext_mask:$Width),
          (BFEXTU GPR:$Src, uimm5:$Bp, bfext_mask:$Width)>;
def : Pat<(ctlz GPR:$Src), (CLZrr GPR:$Src)>;
def : Pat<(ctlz_zero_undef GPR:$Src), (CLZrr GPR:$Src)>;
def : Pat<(cttz GPR:$Src), (CLZrr (BREVcg GPR:$Src))>;
def : Pat<(cttz_zero_undef GPR:$Src), (CLZrr (BREVcg GPR:$Src))>;
def : Pat<(mul GPR:$LHS, GPR:$RHS), (MULrrr GPR:$LHS, GPR:$RHS)>;
def : Pat<(sext_inreg GPR:$Src, i8),
          (CASTS_Bcg GPR:$Src)>;
def : Pat<(sext_inreg GPR:$Src, i16),
          (CASTS_Hcg GPR:$Src)>;

def : Pat<(AVR32cmp GPR:$LHS, GPR:$RHS), (CPrr GPR:$LHS, GPR:$RHS)>;
def : Pat<(AVR32cmp GPR:$LHS, simm21:$RHS), (CPri21 GPR:$LHS, simm21:$RHS)>;
def : Pat<(AVR32cmpc GPR:$LHS, GPR:$RHS), (CPCrr GPR:$LHS, GPR:$RHS)>;

def : Pat<(AVR32brcc bb:$dst, AVR32_COND_EQ), (BREQbb bb:$dst)>;
def : Pat<(AVR32brcc bb:$dst, AVR32_COND_NE), (BRNEbb bb:$dst)>;
def : Pat<(AVR32brcc bb:$dst, AVR32_COND_CC), (BRCCbb bb:$dst)>;
def : Pat<(AVR32brcc bb:$dst, AVR32_COND_CS), (BRCSbb bb:$dst)>;
def : Pat<(AVR32brcc bb:$dst, AVR32_COND_GE), (BRGEbb bb:$dst)>;
def : Pat<(AVR32brcc bb:$dst, AVR32_COND_LT), (BRLTbb bb:$dst)>;
def : Pat<(AVR32brcc bb:$dst, AVR32_COND_LS), (BRLSbb bb:$dst)>;
def : Pat<(AVR32brcc bb:$dst, AVR32_COND_GT), (BRGTbb bb:$dst)>;
def : Pat<(AVR32brcc bb:$dst, AVR32_COND_LE), (BRLEbb bb:$dst)>;
def : Pat<(AVR32brcc bb:$dst, AVR32_COND_HI), (BRHIbb bb:$dst)>;
