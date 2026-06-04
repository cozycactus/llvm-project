//===-- AVR32Subtarget.cpp - AVR32 subtarget information -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32Subtarget.h"

using namespace llvm;

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AVR32GenSubtargetInfo.inc"

void AVR32Subtarget::anchor() {}

AVR32Subtarget &
AVR32Subtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS) {
  StringRef CPUName = CPU.empty() ? "generic" : CPU;
  ParseSubtargetFeatures(CPUName, /*TuneCPU*/ CPUName, FS);
  return *this;
}

AVR32Subtarget::AVR32Subtarget(const Triple &TT, const std::string &CPU,
                               const std::string &FS, const TargetMachine &TM)
    : AVR32GenSubtargetInfo(TT, CPU, /*TuneCPU*/ CPU, FS),
      InstrInfo(initializeSubtargetDependencies(CPU, FS)), FrameLowering(*this),
      TLInfo(TM, *this), TSInfo() {}
