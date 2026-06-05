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

namespace AVR32CC {
enum CondCodes {
  COND_EQ = 0,
  COND_NE = 1,
  COND_CC = 2,
  COND_CS = 3,
  COND_GE = 4,
  COND_LT = 5,
  COND_MI = 6,
  COND_PL = 7,
  COND_LS = 8,
  COND_GT = 9,
  COND_LE = 10,
  COND_HI = 11,
  COND_VS = 12,
  COND_VC = 13,
  COND_QS = 14,
  COND_AL = 15,
  COND_INVALID = -1
};
} // namespace AVR32CC

namespace AVR32II {
enum TargetFlags { MO_NO_FLAG = 0, MO_ABS_HI, MO_ABS_LO };
} // namespace AVR32II

namespace llvm {
class AVR32TargetMachine;
class FunctionPass;
class PassRegistry;

FunctionPass *createAVR32ISelDag(AVR32TargetMachine &TM,
                                 CodeGenOptLevel OptLevel);
FunctionPass *createAVR32PeepholePass();
void initializeAVR32AsmPrinterPass(PassRegistry &);
void initializeAVR32DAGToDAGISelLegacyPass(PassRegistry &);
void initializeAVR32PeepholePass(PassRegistry &);
} // namespace llvm

#endif // LLVM_LIB_TARGET_AVR32_AVR32_H
