//===-- AVR32FrameLowering.cpp - AVR32 frame lowering ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32FrameLowering.h"
#include "AVR32InstrInfo.h"
#include "AVR32Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "AVR32GenInstrInfo.inc"
#define GET_REGINFO_ENUM
#include "AVR32GenRegisterInfo.inc"

AVR32FrameLowering::AVR32FrameLowering(const AVR32Subtarget &STI)
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(4), 0,
                          Align(4)) {}

bool AVR32FrameLowering::hasFPImpl(const MachineFunction &MF) const {
  return false;
}

void AVR32FrameLowering::emitPrologue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {
  uint64_t StackSize = MF.getFrameInfo().getStackSize();
  if (StackSize == 0)
    return;

  if (StackSize % 4 != 0 || StackSize > 508)
    report_fatal_error("AVR32 stack frame size is not supported yet");

  const AVR32InstrInfo &TII =
      *MF.getSubtarget<AVR32Subtarget>().getInstrInfo();
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  BuildMI(MBB, MBBI, DL, TII.get(AVR32::SUBSPri8), AVR32::SP)
      .addImm(StackSize);
}

void AVR32FrameLowering::emitEpilogue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {
  uint64_t StackSize = MF.getFrameInfo().getStackSize();
  if (StackSize == 0)
    return;

  if (StackSize % 4 != 0 || StackSize > 512)
    report_fatal_error("AVR32 stack frame size is not supported yet");

  const AVR32InstrInfo &TII =
      *MF.getSubtarget<AVR32Subtarget>().getInstrInfo();
  MachineBasicBlock::iterator MBBI = MBB.getFirstTerminator();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  BuildMI(MBB, MBBI, DL, TII.get(AVR32::SUBSPri8), AVR32::SP)
      .addImm(-static_cast<int64_t>(StackSize));
}

MachineBasicBlock::iterator AVR32FrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  return MBB.erase(I);
}
