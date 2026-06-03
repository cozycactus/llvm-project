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

foreach I = 0-12 in
  def R#I : AVR32Reg<I, "r"#I, ["R"#I]>, DwarfRegNum<[I]>;

def SP : AVR32Reg<13, "sp", ["SP", "r13", "R13"]>, DwarfRegNum<[13]>;
def LR : AVR32Reg<14, "lr", ["LR", "r14", "R14"]>, DwarfRegNum<[14]>;
def PC : AVR32Reg<15, "pc", ["PC", "r15", "R15"]>, DwarfRegNum<[15]>;

def GPR : RegisterClass<"AVR32", [i32], 32,
                         (add (sequence "R%u", 0, 12), SP, LR, PC)>;
