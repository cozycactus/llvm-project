//===-- AVR32InstPrinter.h - AVR32 MCInst to assembly syntax ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR32_MCTARGETDESC_AVR32INSTPRINTER_H
#define LLVM_LIB_TARGET_AVR32_MCTARGETDESC_AVR32INSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"

namespace llvm {

class AVR32InstPrinter : public MCInstPrinter {
public:
  AVR32InstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                   const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}

  void printRegName(raw_ostream &OS, MCRegister Reg) override;

  void printInst(const MCInst *MI, uint64_t Address, StringRef Annot,
                 const MCSubtargetInfo &STI, raw_ostream &OS) override;

  std::pair<const char *, uint64_t>
  getMnemonic(const MCInst &MI) const override;
  void printInstruction(const MCInst *MI, uint64_t Address, raw_ostream &OS);
  bool printAliasInstr(const MCInst *MI, uint64_t Address, raw_ostream &OS);
  void printCustomAliasOperand(const MCInst *MI, uint64_t Address,
                               unsigned OpIdx, unsigned PrintMethodIdx,
                               raw_ostream &OS);
  static const char *getRegisterName(MCRegister Reg);

private:
  void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &OS);
  void printHalfPart(const MCInst *MI, unsigned OpNo, raw_ostream &OS);
  void printBytePart(const MCInst *MI, unsigned OpNo, raw_ostream &OS);
  void printRegList16(const MCInst *MI, unsigned OpNo, raw_ostream &OS);
  void printRegList8(const MCInst *MI, unsigned OpNo, raw_ostream &OS);
};

} // end namespace llvm

#endif
