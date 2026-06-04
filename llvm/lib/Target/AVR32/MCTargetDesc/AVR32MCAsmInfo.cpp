//===-- AVR32MCAsmInfo.cpp - AVR32 Asm Properties -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32MCAsmInfo.h"

using namespace llvm;

void AVR32MCAsmInfo::anchor() {}

AVR32MCAsmInfo::AVR32MCAsmInfo(const MCTargetOptions &Options)
    : MCAsmInfoELF(Options) {
  IsLittleEndian = false;
  CodePointerSize = 4;
  CalleeSaveStackSlotSize = 4;
  MinInstAlignment = 2;

  CommentString = "#";
  SupportsDebugInformation = true;
  ExceptionsType = ExceptionHandling::DwarfCFI;
}
