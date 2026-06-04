//===-- AVR32.h - Top-level interface for AVR32 target ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR32_AVR32_H
#define LLVM_LIB_TARGET_AVR32_AVR32_H

#include "llvm/Support/CodeGen.h"

namespace llvm {
class AVR32TargetMachine;
class FunctionPass;
class PassRegistry;

FunctionPass *createAVR32ISelDag(AVR32TargetMachine &TM,
                                 CodeGenOptLevel OptLevel);
void initializeAVR32AsmPrinterPass(PassRegistry &);
void initializeAVR32DAGToDAGISelLegacyPass(PassRegistry &);
} // namespace llvm

#endif // LLVM_LIB_TARGET_AVR32_AVR32_H
