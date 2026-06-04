//===-- AVR32InstPrinter.cpp - AVR32 MCInst to assembly syntax ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32InstPrinter.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#define PRINT_ALIAS_INSTR
#include "../AVR32GenAsmWriter.inc"

void AVR32InstPrinter::printRegName(raw_ostream &OS, MCRegister Reg) {
  OS << getRegisterName(Reg);
}

void AVR32InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                 StringRef Annot, const MCSubtargetInfo &STI,
                                 raw_ostream &OS) {
  if (!printAliasInstr(MI, Address, OS))
    printInstruction(MI, Address, OS);
  printAnnotation(OS, Annot);
}

void AVR32InstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                    raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    OS << getRegisterName(Op.getReg());
    return;
  }
  if (Op.isImm()) {
    OS << formatImm(Op.getImm());
    return;
  }
  assert(Op.isExpr() && "unknown AVR32 operand kind");
  MAI.printExpr(OS, *Op.getExpr());
}

void AVR32InstPrinter::printHalfPart(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  assert(Op.isImm() && "halfword part selector must be immediate");
  OS << (Op.getImm() ? "t" : "b");
}

void AVR32InstPrinter::printBytePart(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  assert(Op.isImm() && "byte part selector must be immediate");
  static const char *Parts[] = {"b", "l", "u", "t"};
  int64_t Part = Op.getImm();
  if (Part < 0 || Part > 3)
    llvm_unreachable("invalid byte part selector");
  OS << Parts[Part];
}

void AVR32InstPrinter::printRegList16(const MCInst *MI, unsigned OpNo,
                                      raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  assert(Op.isImm() && "register list must be immediate mask");
  static const char *Names[] = {"r0",  "r1", "r2", "r3", "r4", "r5",
                               "r6",  "r7", "r8", "r9", "r10", "r11",
                               "r12", "sp", "lr", "pc"};
  uint16_t Mask = static_cast<uint16_t>(Op.getImm());
  bool NeedComma = false;

  for (unsigned Start = 0; Start < 16;) {
    if ((Mask & (1u << Start)) == 0) {
      ++Start;
      continue;
    }

    unsigned End = Start;
    while (End + 1 < 16 && (Mask & (1u << (End + 1))))
      ++End;

    if (NeedComma)
      OS << ", ";
    OS << Names[Start];
    if (End != Start)
      OS << "-" << Names[End];
    NeedComma = true;
    Start = End + 1;
  }
}

void AVR32InstPrinter::printRegList8(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  assert(Op.isImm() && "register list must be immediate mask");
  static const char *Groups[] = {"r0-r3", "r4-r7", "r8-r9", "r10",
                                 "r11",   "r12",   "lr",    "pc"};
  uint8_t Mask = static_cast<uint8_t>(Op.getImm());
  bool NeedComma = false;

  for (unsigned Bit = 0; Bit < 8; ++Bit) {
    if ((Mask & (1u << Bit)) == 0)
      continue;
    if (NeedComma)
      OS << ", ";
    OS << Groups[Bit];
    NeedComma = true;
  }
}

void AVR32InstPrinter::printCoprocessor(const MCInst *MI, unsigned OpNo,
                                        raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  assert(Op.isImm() && "coprocessor must be immediate");
  OS << "cp" << Op.getImm();
}

void AVR32InstPrinter::printCoprocessorRegister(const MCInst *MI,
                                                unsigned OpNo,
                                                raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  assert(Op.isImm() && "coprocessor register must be immediate");
  OS << "cr" << Op.getImm();
}

void AVR32InstPrinter::printPicoIn(const MCInst *MI, unsigned OpNo,
                                   raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  assert(Op.isImm() && "PICO input must be immediate");
  OS << "in" << Op.getImm();
}

static void printCoprocessorRegListMask(uint16_t Mask, unsigned BaseReg,
                                        unsigned Scale, raw_ostream &OS) {
  bool NeedComma = false;
  for (unsigned Start = 0; Start < 8;) {
    if ((Mask & (1u << Start)) == 0) {
      ++Start;
      continue;
    }

    unsigned End = Start;
    while (End + 1 < 8 && (Mask & (1u << (End + 1))))
      ++End;

    if (NeedComma)
      OS << ", ";
    OS << "cr" << (BaseReg + Start * Scale);
    unsigned EndReg = BaseReg + End * Scale + Scale - 1;
    if (EndReg != BaseReg + Start * Scale)
      OS << "-cr" << EndReg;
    NeedComma = true;
    Start = End + 1;
  }
}

void AVR32InstPrinter::printCoprocessorRegListD(const MCInst *MI,
                                                unsigned OpNo,
                                                raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  assert(Op.isImm() && "coprocessor register list must be immediate mask");
  printCoprocessorRegListMask(static_cast<uint8_t>(Op.getImm()), 0, 2, OS);
}

void AVR32InstPrinter::printCoprocessorRegListLow(const MCInst *MI,
                                                  unsigned OpNo,
                                                  raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  assert(Op.isImm() && "coprocessor register list must be immediate mask");
  printCoprocessorRegListMask(static_cast<uint8_t>(Op.getImm()), 0, 1, OS);
}

void AVR32InstPrinter::printCoprocessorRegListHigh(const MCInst *MI,
                                                   unsigned OpNo,
                                                   raw_ostream &OS) {
  const MCOperand &Op = MI->getOperand(OpNo);
  assert(Op.isImm() && "coprocessor register list must be immediate mask");
  printCoprocessorRegListMask(static_cast<uint16_t>(Op.getImm()) >> 8, 8, 1,
                              OS);
}
