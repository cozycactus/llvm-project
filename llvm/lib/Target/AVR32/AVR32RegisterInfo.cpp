//===-- AVR32RegisterInfo.cpp - AVR32 register information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32RegisterInfo.h"
#include "AVR32FrameLowering.h"
#include "AVR32Subtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "AVR32GenInstrInfo.inc"
#define GET_REGINFO_ENUM
#include "AVR32GenRegisterInfo.inc"
#define GET_REGINFO_TARGET_DESC
#include "AVR32GenRegisterInfo.inc"

AVR32RegisterInfo::AVR32RegisterInfo() : AVR32GenRegisterInfo(AVR32::LR) {}

const MCPhysReg *
AVR32RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  static const MCPhysReg CalleeSavedRegs[] = {0};
  return CalleeSavedRegs;
}

const uint32_t *
AVR32RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                        CallingConv::ID CC) const {
  return CSR_Normal_RegMask;
}

BitVector AVR32RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(AVR32::SP);
  Reserved.set(AVR32::PC);
  // Non-leaf functions save LR in PUSHM, so it can be a caller-clobbered
  // scratch register between calls. Leaf functions still return through LR.
  if (!MF.getFrameInfo().hasCalls())
    Reserved.set(AVR32::LR);
  const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
  if (TFI->hasFP(MF))
    Reserved.set(AVR32::R7);
  return Reserved;
}

const TargetRegisterClass *
AVR32RegisterInfo::getPointerRegClass(unsigned Kind) const {
  return &AVR32::GPRRegClass;
}

bool AVR32RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                            int SPAdj, unsigned FIOperandNum,
                                            RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected");

  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  Register FrameReg = getFrameRegister(MF);

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  int64_t Offset = MFI.getObjectOffset(FrameIndex) + MFI.getStackSize();

  Offset += MI.getOperand(FIOperandNum + 1).getImm();
  if (!isInt<16>(Offset))
    report_fatal_error("AVR32 frame offset does not fit in a 16-bit immediate");

  if (MI.getOpcode() == AVR32::FIADDR) {
    if (!isInt<16>(-Offset))
      report_fatal_error("AVR32 frame address offset does not fit in a 16-bit "
                         "immediate");

    MachineBasicBlock &MBB = *MI.getParent();
    const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
    DebugLoc DL = MI.getDebugLoc();
    Register DestReg = MI.getOperand(0).getReg();
    BuildMI(MBB, II, DL, TII.get(AVR32::SUBrri16), DestReg)
        .addReg(FrameReg)
        .addImm(-Offset);
    MI.eraseFromParent();
    return false;
  }

  MI.getOperand(FIOperandNum).ChangeToRegister(FrameReg, false);
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
  return false;
}

Register AVR32RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
  return TFI->hasFP(MF) ? AVR32::R7 : AVR32::SP;
}
