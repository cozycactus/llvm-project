//===-- AVR32TargetInfo.cpp - AVR32 Target Implementation -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32TargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheAVR32Target() {
  static Target TheAVR32Target;
  return TheAVR32Target;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVR32TargetInfo() {
  RegisterTarget<Triple::avr32, /*HasJIT=*/false> X(
      getTheAVR32Target(), "avr32", "Atmel AVR32", "AVR32");
}
