//===-- AVR32InstrFormats.td - AVR32 Instruction Formats --*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class AVR32Inst<bits<16> Enc, dag Outs, dag Ins, string AsmStr,
                list<dag> Patterns = []> : Instruction {
  field bits<16> Inst = Enc;

  let Namespace = "AVR32";
  let Size = 2;
  let OutOperandList = Outs;
  let InOperandList = Ins;
  let AsmString = AsmStr;
  let Pattern = Patterns;
}
