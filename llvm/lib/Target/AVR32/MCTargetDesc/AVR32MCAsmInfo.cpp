//===-- AVR32MCAsmInfo.cpp - AVR32 Asm Properties -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32MCAsmInfo.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

void AVR32MCAsmInfo::anchor() {}

AVR32MCAsmInfo::AVR32MCAsmInfo(const MCTargetOptions &Options)
    : MCAsmInfoELF() {
  IsLittleEndian = false;
  CodePointerSize = 4;
  CalleeSaveStackSlotSize = 4;
  MinInstAlignment = 2;

  CommentString = "#";
  DollarIsPC = true;
  UsesELFSectionDirectiveForBSS = true;
  SupportsDebugInformation = true;
  ExceptionsType = ExceptionHandling::DwarfCFI;
}

void AVR32MCAsmInfo::printSpecifierExpr(raw_ostream &OS,
                                        const MCSpecifierExpr &Expr) const {
  switch (Expr.getSpecifier()) {
  case ELF::R_AVR32_HI16:
    OS << "HI(";
    break;
  case ELF::R_AVR32_LO16:
    OS << "LO(";
    break;
  case ELF::R_AVR32_32_CPENT:
    printExpr(OS, *Expr.getSubExpr());
    return;
  default:
    llvm_unreachable("Unsupported AVR32 expression specifier");
  }
  printExpr(OS, *Expr.getSubExpr());
  OS << ')';
}
