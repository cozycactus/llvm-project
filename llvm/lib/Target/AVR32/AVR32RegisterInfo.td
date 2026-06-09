//===-- AVR32RegisterInfo.td - AVR32 Register Definitions -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class AVR32Reg<bits<16> Enc, string Name, list<string> AltNames = []>
  : Register<Name, AltNames> {
  let HWEncoding = Enc;
  let Namespace = "AVR32";
}

def sub_lo : SubRegIndex<32>;
def sub_hi : SubRegIndex<32, 32>;

foreach I = 0-12 in
  def R#I : AVR32Reg<I, "r"#I, ["R"#I]>, DwarfRegNum<[I]>;

def SP : AVR32Reg<13, "sp", ["SP", "r13", "R13"]>, DwarfRegNum<[13]>;
def LR : AVR32Reg<14, "lr", ["LR", "r14", "R14"]>, DwarfRegNum<[14]>;
def PC : AVR32Reg<15, "pc", ["PC", "r15", "R15"]>, DwarfRegNum<[15]>;
def SREG : AVR32Reg<16, "sreg", ["SREG"]>, DwarfRegNum<[16]>;

def GPR : RegisterClass<"AVR32", [i32], 32,
                         (add (sequence "R%u", 8, 12),
                              (sequence "R%u", 4, 7),
                              (sequence "R%u", 0, 3), SP, LR, PC)>;

def GPRNoSP : RegisterClass<"AVR32", [i32], 32,
                             (add (sequence "R%u", 8, 12),
                                  (sequence "R%u", 4, 7),
                                  (sequence "R%u", 0, 3), LR, PC)>;

def GPREven : RegisterClass<"AVR32", [i32], 32,
                            (add R8, R10, R12, R4, R6, R0, R2, LR)>;

def GPRDivPairs : RegisterTuples<[sub_lo, sub_hi],
                                 [(add R8, R10, R4, R6, R0, R2),
                                  (add R9, R11, R5, R7, R1, R3)]>;

def GPRDivLo : RegisterClass<"AVR32", [i32], 32,
                             (add R8, R10, R4, R6, R0, R2)>;

def GPRDivPair : RegisterClass<"AVR32", [untyped], 64, (add GPRDivPairs)> {
  let Size = 64;
}

def SPOnly : RegisterClass<"AVR32", [i32], 32, (add SP)>;

def PCOnly : RegisterClass<"AVR32", [i32], 32, (add PC)>;

def CSR_Normal : CalleeSavedRegs<(add (sequence "R%u", 0, 7))>;
