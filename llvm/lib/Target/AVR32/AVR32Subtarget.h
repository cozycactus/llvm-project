//===-- AVR32Subtarget.h - Define subtarget for AVR32 ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR32_AVR32SUBTARGET_H
#define LLVM_LIB_TARGET_AVR32_AVR32SUBTARGET_H

#include "AVR32FrameLowering.h"
#include "AVR32ISelLowering.h"
#include "AVR32InstrInfo.h"
#include "AVR32SelectionDAGInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"

#define GET_SUBTARGETINFO_HEADER
#include "AVR32GenSubtargetInfo.inc"

namespace llvm {
class LibcallLoweringInfo;
class StringRef;

class AVR32Subtarget : public AVR32GenSubtargetInfo {
  virtual void anchor();

  bool EnableLinkerRelax = false;
  bool HasCondStore = false;

  AVR32InstrInfo InstrInfo;
  AVR32FrameLowering FrameLowering;
  AVR32TargetLowering TLInfo;
  AVR32SelectionDAGInfo TSInfo;

public:
  AVR32Subtarget(const Triple &TT, const std::string &CPU,
                 const std::string &FS, const TargetMachine &TM);

  AVR32Subtarget &initializeSubtargetDependencies(StringRef CPU, StringRef FS);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const AVR32InstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const AVR32FrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const AVR32TargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const AVR32SelectionDAGInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }
  void initLibcallLoweringInfo(LibcallLoweringInfo &Info) const override;
  const AVR32RegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  bool hasCondStore() const { return HasCondStore; }
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_AVR32_AVR32SUBTARGET_H
