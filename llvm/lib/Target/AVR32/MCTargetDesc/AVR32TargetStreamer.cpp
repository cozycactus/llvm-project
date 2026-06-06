//===-- AVR32TargetStreamer.cpp - AVR32 Target Streamer -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32TargetStreamer.h"
#include "AVR32MCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/ConstantPools.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

AVR32TargetStreamer::AVR32TargetStreamer(MCStreamer &S)
    : MCTargetStreamer(S), ConstantPools(new AssemblerConstantPools()) {}

AVR32TargetStreamer::~AVR32TargetStreamer() = default;

const MCExpr *AVR32TargetStreamer::addCPENTConstantPoolEntry(const MCExpr *Expr,
                                                             SMLoc Loc) {
  if (!isa<MCConstantExpr>(Expr))
    Expr =
        MCSpecifierExpr::create(Expr, ELF::R_AVR32_32_CPENT, getContext(), Loc);
  return ConstantPools->addEntry(Streamer, Expr, 4, Loc);
}

void AVR32TargetStreamer::emitCurrentConstantPool() {
  ConstantPools->emitForCurrentSection(Streamer);
  ConstantPools->clearCacheForCurrentSection(Streamer);
}

void AVR32TargetStreamer::emitConstantPools() {
  ConstantPools->emitAll(Streamer);
}

void AVR32TargetStreamer::finish() {
  emitConstantPools();
}

MCTargetStreamer *llvm::createAVR32NullTargetStreamer(MCStreamer &S) {
  return new AVR32TargetStreamer(S);
}

MCTargetStreamer *llvm::createAVR32AsmTargetStreamer(MCStreamer &S,
                                                     formatted_raw_ostream &OS,
                                                     MCInstPrinter *InstPrint) {
  return new AVR32TargetStreamer(S);
}

MCTargetStreamer *
llvm::createAVR32ObjectTargetStreamer(MCStreamer &S,
                                      const MCSubtargetInfo &STI) {
  if (STI.hasFeature(AVR32::FeatureRelax)) {
    ELFObjectWriter &W = static_cast<MCELFStreamer &>(S).getWriter();
    W.setELFHeaderEFlags(W.getELFHeaderEFlags() | ELF::EF_AVR32_LINKRELAX);
  }
  return new AVR32TargetStreamer(S);
}
