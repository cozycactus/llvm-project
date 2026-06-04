//===-- AVR32FrameLowering.cpp - AVR32 frame lowering ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32FrameLowering.h"
#include "llvm/CodeGen/MachineFunction.h"

using namespace llvm;

AVR32FrameLowering::AVR32FrameLowering(const AVR32Subtarget &STI)
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(4), 0,
                          Align(4)) {}

bool AVR32FrameLowering::hasFPImpl(const MachineFunction &MF) const {
  return false;
}

void AVR32FrameLowering::emitPrologue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {}

void AVR32FrameLowering::emitEpilogue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {}

MachineBasicBlock::iterator AVR32FrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  return MBB.erase(I);
}
