//===-- AVR32TargetMachine.h - Define target machine for AVR32 -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR32_AVR32TARGETMACHINE_H
#define LLVM_LIB_TARGET_AVR32_AVR32TARGETMACHINE_H

#include "AVR32Subtarget.h"
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include <optional>

namespace llvm {
class TargetTransformInfo;

class AVR32TargetMachine : public CodeGenTargetMachineImpl {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  AVR32Subtarget Subtarget;

public:
  AVR32TargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                     StringRef FS, const TargetOptions &Options,
                     std::optional<Reloc::Model> RM,
                     std::optional<CodeModel::Model> CM, CodeGenOptLevel OL,
                     bool JIT);
  ~AVR32TargetMachine() override;

  const AVR32Subtarget *getSubtargetImpl(const Function &F) const override {
    return &Subtarget;
  }

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  TargetTransformInfo getTargetTransformInfo(const Function &F) const override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_AVR32_AVR32TARGETMACHINE_H
