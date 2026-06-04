//===-- AVR32RegisterInfo.cpp - AVR32 register information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32RegisterInfo.h"
#include "AVR32FrameLowering.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_ENUM
#include "AVR32GenRegisterInfo.inc"
#define GET_REGINFO_TARGET_DESC
#include "AVR32GenRegisterInfo.inc"

AVR32RegisterInfo::AVR32RegisterInfo() : AVR32GenRegisterInfo(AVR32::LR) {}

const MCPhysReg *
AVR32RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return nullptr;
}

BitVector AVR32RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(AVR32::SP);
  Reserved.set(AVR32::LR);
  Reserved.set(AVR32::PC);
  return Reserved;
}

const TargetRegisterClass *
AVR32RegisterInfo::getPointerRegClass(unsigned Kind) const {
  return &AVR32::GPRRegClass;
}

bool AVR32RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                            int SPAdj, unsigned FIOperandNum,
                                            RegScavenger *RS) const {
  llvm_unreachable("AVR32 frame index elimination is not implemented yet");
}

Register AVR32RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return AVR32::SP;
}
