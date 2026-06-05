//===-- AVR32TargetMachine.cpp - Define target machine for AVR32 ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32TargetMachine.h"
#include "AVR32.h"
#include "TargetInfo/AVR32TargetInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

static Reloc::Model getEffectiveRelocModel(std::optional<Reloc::Model> RM) {
  return RM.value_or(Reloc::Static);
}

AVR32TargetMachine::AVR32TargetMachine(
    const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
    const TargetOptions &Options, std::optional<Reloc::Model> RM,
    std::optional<CodeModel::Model> CM, CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, "E-m:e-p:32:32-i64:32-n32-S32", TT, CPU, FS,
                               Options, getEffectiveRelocModel(RM),
                               getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(TT, std::string(CPU), std::string(FS), *this) {
  initAsmInfo();
}

AVR32TargetMachine::~AVR32TargetMachine() = default;

namespace {
class AVR32PassConfig : public TargetPassConfig {
public:
  AVR32PassConfig(AVR32TargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  AVR32TargetMachine &getAVR32TargetMachine() const {
    return getTM<AVR32TargetMachine>();
  }

  bool addInstSelector() override;
  void addPreEmitPass() override;
};
} // namespace

TargetPassConfig *AVR32TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new AVR32PassConfig(*this, PM);
}

bool AVR32PassConfig::addInstSelector() {
  addPass(createAVR32ISelDag(getAVR32TargetMachine(), getOptLevel()));
  return false;
}

void AVR32PassConfig::addPreEmitPass() { addPass(createAVR32PeepholePass()); }

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeAVR32Target() {
  RegisterTargetMachine<AVR32TargetMachine> X(getTheAVR32Target());
  PassRegistry &PR = *PassRegistry::getPassRegistry();
  initializeAVR32AsmPrinterPass(PR);
  initializeAVR32DAGToDAGISelLegacyPass(PR);
  initializeAVR32PeepholePass(PR);
}
