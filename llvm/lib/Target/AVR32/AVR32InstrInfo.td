//===-- AVR32InstrInfo.td - AVR32 Instruction Definitions -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "AVR32InstrFormats.td"

def NOP : AVR32Inst<0b1101011100000011, (outs), (ins), "nop">;
