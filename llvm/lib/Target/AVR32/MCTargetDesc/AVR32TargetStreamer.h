//===-- AVR32TargetStreamer.h - AVR32 Target Streamer ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR32_MCTARGETDESC_AVR32TARGETSTREAMER_H
#define LLVM_LIB_TARGET_AVR32_MCTARGETDESC_AVR32TARGETSTREAMER_H

#include "llvm/MC/MCStreamer.h"
#include <memory>

namespace llvm {

class AssemblerConstantPools;
class MCExpr;

class AVR32TargetStreamer : public MCTargetStreamer {
  std::unique_ptr<AssemblerConstantPools> ConstantPools;

public:
  AVR32TargetStreamer(MCStreamer &S);
  ~AVR32TargetStreamer() override;

  const MCExpr *addCPENTConstantPoolEntry(const MCExpr *Expr, SMLoc Loc);
  void emitCurrentConstantPool();
  void emitConstantPools() override;
};

} // end namespace llvm

#endif
