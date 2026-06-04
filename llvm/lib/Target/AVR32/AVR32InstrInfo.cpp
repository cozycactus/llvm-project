//===-- AVR32InstrInfo.cpp - AVR32 instruction information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32InstrInfo.h"
#include "AVR32Subtarget.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "AVR32GenInstrInfo.inc"
#define GET_INSTRINFO_CTOR_DTOR
#include "AVR32GenInstrInfo.inc"

AVR32InstrInfo::AVR32InstrInfo(const AVR32Subtarget &STI)
    : AVR32GenInstrInfo(STI, RI), RI() {}

void AVR32InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator I,
                                 const DebugLoc &DL, Register DestReg,
                                 Register SrcReg, bool KillSrc,
                                 bool RenamableDest,
                                 bool RenamableSrc) const {
  if (!AVR32::GPRRegClass.contains(DestReg, SrcReg))
    llvm_unreachable("AVR32 can only copy GPR registers");

  BuildMI(MBB, I, DL, get(AVR32::MOVrr), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
}

void AVR32InstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool isKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
    MachineInstr::MIFlag Flags) const {
  llvm_unreachable("AVR32 stack stores are not implemented yet");
}

void AVR32InstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MI,
                                          Register DestReg, int FrameIdx,
                                          const TargetRegisterClass *RC,
                                          Register VReg, unsigned SubReg,
                                          MachineInstr::MIFlag Flags) const {
  llvm_unreachable("AVR32 stack loads are not implemented yet");
}
