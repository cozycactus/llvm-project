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
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "AVR32GenInstrInfo.inc"
#define GET_REGINFO_ENUM
#include "AVR32GenRegisterInfo.inc"

static unsigned getPushmMask(const MachineFunction &MF) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const MachineRegisterInfo &MRI = MF.getRegInfo();
  bool IsInterrupt = MF.getFunction().hasFnAttribute("interrupt");
  unsigned Mask = 0;
  if (MRI.isPhysRegModified(AVR32::R0) || MRI.isPhysRegModified(AVR32::R1) ||
      MRI.isPhysRegModified(AVR32::R2) || MRI.isPhysRegModified(AVR32::R3))
    Mask |= 1 << 0;
  if (MRI.isPhysRegModified(AVR32::R4) || MRI.isPhysRegModified(AVR32::R5) ||
      MRI.isPhysRegModified(AVR32::R6) || MRI.isPhysRegModified(AVR32::R7) ||
      MFI.hasVarSizedObjects())
    Mask |= 1 << 1;
  if (IsInterrupt &&
      (MRI.isPhysRegModified(AVR32::R8) || MRI.isPhysRegModified(AVR32::R9)))
    Mask |= 1 << 2;
  if (IsInterrupt && MRI.isPhysRegModified(AVR32::R10))
    Mask |= 1 << 3;
  if (IsInterrupt && MRI.isPhysRegModified(AVR32::R11))
    Mask |= 1 << 4;
  if (IsInterrupt && MRI.isPhysRegModified(AVR32::R12))
    Mask |= 1 << 5;
  if (MFI.hasCalls())
    Mask |= 1 << 6;
  return Mask;
}

AVR32FrameLowering::AVR32FrameLowering(const AVR32Subtarget &STI)
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(4), 0,
                          Align(4)) {}

bool AVR32FrameLowering::hasFPImpl(const MachineFunction &MF) const {
  return MF.getFrameInfo().hasVarSizedObjects();
}

void AVR32FrameLowering::emitPrologue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {
  uint64_t StackSize = MF.getFrameInfo().getStackSize();
  bool HasFP = hasFP(MF);
  unsigned PushmMask = getPushmMask(MF);
  if (StackSize == 0 && !HasFP && PushmMask == 0)
    return;

  if (StackSize % 4 != 0 || StackSize > 508)
    report_fatal_error("AVR32 stack frame size is not supported yet");

  const AVR32InstrInfo &TII =
      *MF.getSubtarget<AVR32Subtarget>().getInstrInfo();
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  if (PushmMask != 0)
    BuildMI(MBB, MBBI, DL, TII.get(AVR32::PUSHM))
        .addImm(PushmMask)
        .setMIFlag(MachineInstr::FrameSetup);

  if (StackSize != 0)
    BuildMI(MBB, MBBI, DL, TII.get(AVR32::SUBSPri8), AVR32::SP)
        .addImm(StackSize)
        .setMIFlag(MachineInstr::FrameSetup);

  if (HasFP)
    BuildMI(MBB, MBBI, DL, TII.get(AVR32::MOVrr), AVR32::R7)
        .addReg(AVR32::SP);
}

void AVR32FrameLowering::emitEpilogue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {
  uint64_t StackSize = MF.getFrameInfo().getStackSize();
  bool HasFP = hasFP(MF);
  unsigned PushmMask = getPushmMask(MF);
  if (StackSize == 0 && !HasFP && PushmMask == 0)
    return;

  if (StackSize % 4 != 0 || StackSize > 512)
    report_fatal_error("AVR32 stack frame size is not supported yet");

  const AVR32InstrInfo &TII =
      *MF.getSubtarget<AVR32Subtarget>().getInstrInfo();
  MachineBasicBlock::iterator MBBI = MBB.getFirstTerminator();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  bool HasPopmRetVal = false;
  int64_t PopmRetVal = 0;
  MachineBasicBlock::iterator PopmRetValMI = MBB.end();
  if (MBBI != MBB.begin() && MBBI != MBB.end() &&
      MBBI->getOpcode() == AVR32::RETR12) {
    MachineBasicBlock::iterator Prev = std::prev(MBBI);
    while (Prev != MBB.begin() && Prev->isDebugInstr())
      --Prev;
    if (!Prev->isDebugInstr() &&
        (Prev->getOpcode() == AVR32::MOVri8 ||
         Prev->getOpcode() == AVR32::MOVri21) &&
        Prev->getOperand(0).isReg() &&
        Prev->getOperand(0).getReg() == AVR32::R12 &&
        Prev->getOperand(1).isImm()) {
      int64_t Imm = Prev->getOperand(1).getImm();
      if (Imm >= -1 && Imm <= 1) {
        HasPopmRetVal = true;
        PopmRetVal = Imm;
        PopmRetValMI = Prev;
      }
    }
  }

  if (HasFP)
    BuildMI(MBB, MBBI, DL, TII.get(AVR32::MOVrr), AVR32::SP)
        .addReg(AVR32::R7);

  if (StackSize != 0)
    BuildMI(MBB, MBBI, DL, TII.get(AVR32::SUBSPri8), AVR32::SP)
        .addImm(-static_cast<int64_t>(StackSize))
        .setMIFlag(MachineInstr::FrameDestroy);

  if (PushmMask != 0) {
    bool CanPopmReturn = PushmMask & (1 << 6);
    if (CanPopmReturn && MBBI != MBB.end() &&
        MBBI->getOpcode() == AVR32::RETR12) {
      unsigned RetMask = (PushmMask & ~(1 << 6)) | (1 << 7);
      if (HasPopmRetVal && (RetMask & ((1 << 5) | (1 << 6))) == 0) {
        BuildMI(MBB, MBBI, DL, TII.get(AVR32::POPM_RETVAL))
            .addImm(RetMask)
            .addImm(PopmRetVal)
            .setMIFlag(MachineInstr::FrameDestroy);
        PopmRetValMI->eraseFromParent();
      } else {
        BuildMI(MBB, MBBI, DL, TII.get(AVR32::POPM_RET))
            .addImm(RetMask)
            .setMIFlag(MachineInstr::FrameDestroy)
            ->copyImplicitOps(MF, *MBBI);
      }
      MBBI->eraseFromParent();
    } else {
      BuildMI(MBB, MBBI, DL, TII.get(AVR32::POPM))
          .addImm(PushmMask)
          .setMIFlag(MachineInstr::FrameDestroy);
    }
  }
}

MachineBasicBlock::iterator AVR32FrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  MachineInstr &MI = *I;
  uint64_t Amount = alignTo(
      static_cast<uint64_t>(MF.getSubtarget().getInstrInfo()->getFrameSize(MI)),
      getStackAlign());
  if (Amount != 0) {
    if (Amount > 508)
      report_fatal_error("AVR32 call frame size is not supported yet");

    const AVR32InstrInfo &TII =
        *MF.getSubtarget<AVR32Subtarget>().getInstrInfo();
    int64_t Adjust = MI.getOpcode() == TII.getCallFrameSetupOpcode()
                         ? static_cast<int64_t>(Amount)
                         : -static_cast<int64_t>(Amount);
    BuildMI(MBB, I, MI.getDebugLoc(), TII.get(AVR32::SUBSPri8), AVR32::SP)
        .addImm(Adjust);
  }

  return MBB.erase(I);
}
