//===-- AVR32MCTargetDesc.cpp - AVR32 Target Descriptions -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32MCTargetDesc.h"
#include "AVR32MCAsmInfo.h"
#include "../TargetInfo/AVR32TargetInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

static MCAsmInfo *createAVR32MCAsmInfo(const MCRegisterInfo &MRI,
                                       const Triple &TT,
                                       const MCTargetOptions &Options) {
  return new AVR32MCAsmInfo(Options);
}

static MCInstrInfo *createAVR32MCInstrInfo() { return new MCInstrInfo(); }

static MCRegisterInfo *createAVR32MCRegisterInfo(const Triple &TT) {
  return new MCRegisterInfo();
}

static MCSubtargetInfo *createAVR32MCSubtargetInfo(const Triple &TT,
                                                   StringRef CPU,
                                                   StringRef FS) {
  return new MCSubtargetInfo(TT, CPU, CPU, FS, /*ProcNames=*/{},
                             /*ProcFeatures=*/{}, /*ProcDesc=*/{},
                             /*WriteProcResTable=*/nullptr,
                             /*WriteLatencyTable=*/nullptr,
                             /*ReadAdvanceTable=*/nullptr,
                             /*Stages=*/nullptr, /*OperandCycles=*/nullptr,
                             /*ForwardingPaths=*/nullptr);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVR32TargetMC() {
  Target &T = getTheAVR32Target();

  TargetRegistry::RegisterMCAsmInfo(T, createAVR32MCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createAVR32MCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createAVR32MCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createAVR32MCSubtargetInfo);
  TargetRegistry::RegisterMCAsmBackend(T, createAVR32MCAsmBackend);
}
