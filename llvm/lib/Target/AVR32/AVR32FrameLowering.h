//===-- AVR32FrameLowering.h - AVR32 frame lowering ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR32_AVR32FRAMELOWERING_H
#define LLVM_LIB_TARGET_AVR32_AVR32FRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
class AVR32Subtarget;

class AVR32FrameLowering : public TargetFrameLowering {
public:
  explicit AVR32FrameLowering(const AVR32Subtarget &STI);

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I) const override;

protected:
  bool hasFPImpl(const MachineFunction &MF) const override;
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_AVR32_AVR32FRAMELOWERING_H
