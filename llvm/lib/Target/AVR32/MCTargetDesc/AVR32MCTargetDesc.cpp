//===-- AVR32MCTargetDesc.cpp - AVR32 Target Descriptions -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32MCTargetDesc.h"
#include "AVR32InstPrinter.h"
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

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "../AVR32GenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "../AVR32GenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "../AVR32GenRegisterInfo.inc"

static MCAsmInfo *createAVR32MCAsmInfo(const MCRegisterInfo &MRI,
                                       const Triple &TT,
                                       const MCTargetOptions &Options) {
  return new AVR32MCAsmInfo(Options);
}

static MCInstrInfo *createAVR32MCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitAVR32MCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createAVR32MCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitAVR32MCRegisterInfo(X, AVR32::LR);
  return X;
}

static MCSubtargetInfo *createAVR32MCSubtargetInfo(const Triple &TT,
                                                   StringRef CPU,
                                                   StringRef FS) {
  return createAVR32MCSubtargetInfoImpl(TT, CPU, /*TuneCPU=*/CPU, FS);
}

static MCInstPrinter *createAVR32MCInstPrinter(const Triple &TT,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0)
    return new AVR32InstPrinter(MAI, MII, MRI);
  return nullptr;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVR32TargetMC() {
  Target &T = getTheAVR32Target();

  TargetRegistry::RegisterMCAsmInfo(T, createAVR32MCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createAVR32MCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createAVR32MCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createAVR32MCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createAVR32MCInstPrinter);
  TargetRegistry::RegisterMCCodeEmitter(T, createAVR32MCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(T, createAVR32MCAsmBackend);
  TargetRegistry::RegisterObjectTargetStreamer(T,
                                               createAVR32ObjectTargetStreamer);
  TargetRegistry::RegisterAsmTargetStreamer(T, createAVR32AsmTargetStreamer);
  TargetRegistry::RegisterNullTargetStreamer(T, createAVR32NullTargetStreamer);
}
