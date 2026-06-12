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

unsigned AVR32FrameLowering::getPushmMask(const MachineFunction &MF) {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const MachineRegisterInfo &MRI = MF.getRegInfo();
  bool IsInterrupt = MF.getFunction().hasFnAttribute("interrupt");
  unsigned Mask = 0;
  if (MRI.isPhysRegModified(AVR32::R0) || MRI.isPhysRegModified(AVR32::R1) ||
      MRI.isPhysRegModified(AVR32::R2) || MRI.isPhysRegModified(AVR32::R3))
    Mask |= 1 << 0;
  bool HasFP = MF.getTarget().Options.DisableFramePointerElim(MF) ||
               MFI.hasVarSizedObjects() || MFI.isFrameAddressTaken();
  if (MRI.isPhysRegModified(AVR32::R4) || MRI.isPhysRegModified(AVR32::R5) ||
      MRI.isPhysRegModified(AVR32::R6) || MRI.isPhysRegModified(AVR32::R7) ||
      HasFP)
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

  // Match AVR32 GCC's single-instruction popm return shape for framed leaf
  // functions too. Linux's early exception path relies on preserving the
  // caller frame pointer across these returns.
  if (HasFP && (Mask & (1 << 1)))
    Mask |= 1 << 6;

  // Keep call frames that only need LR in the same r4-r7,lr shape that AVR32
  // GCC uses. Linux/QEMU exercises this return frame heavily during early boot.
  if ((Mask & (1 << 6)) && !(Mask & ((1 << 0) | (1 << 1))))
    Mask |= 1 << 1;
  return Mask;
}

unsigned AVR32FrameLowering::getPushmByteSize(const MachineFunction &MF) {
  unsigned Mask = getPushmMask(MF);
  unsigned RegCount = 0;
  if (Mask & (1 << 0))
    RegCount += 4; // r0-r3
  if (Mask & (1 << 1))
    RegCount += 4; // r4-r7
  if (Mask & (1 << 2))
    RegCount += 2; // r8-r9
  if (Mask & (1 << 3))
    RegCount += 1; // r10
  if (Mask & (1 << 4))
    RegCount += 1; // r11
  if (Mask & (1 << 5))
    RegCount += 1; // r12
  if (Mask & (1 << 6))
    RegCount += 1; // lr
  if (Mask & (1 << 7))
    RegCount += 1; // pc, used by popm-return forms.
  return RegCount * 4;
}

AVR32FrameLowering::AVR32FrameLowering(const AVR32Subtarget &STI)
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(4), 0,
                          Align(4)) {}

static void emitSPAdjustment(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator MBBI,
                             const DebugLoc &DL, const AVR32InstrInfo &TII,
                             int64_t Amount, MachineInstr::MIFlag Flag) {
  if (Amount == 0)
    return;
  if (Amount % 4 != 0 || !isInt<21>(Amount))
    report_fatal_error("AVR32 stack frame size is not supported yet");

  unsigned Opc = Amount >= -512 && Amount <= 508 ? AVR32::SUBSPri8
                                                  : AVR32::SUBri21;
  BuildMI(MBB, MBBI, DL, TII.get(Opc), AVR32::SP)
      .addImm(Amount)
      .setMIFlag(Flag);
}

bool AVR32FrameLowering::hasFPImpl(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MFI.hasVarSizedObjects() || MFI.isFrameAddressTaken();
}

void AVR32FrameLowering::emitPrologue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {
  uint64_t StackSize = MF.getFrameInfo().getStackSize();
  bool HasFP = hasFP(MF);
  unsigned PushmMask = getPushmMask(MF);
  if (StackSize == 0 && !HasFP && PushmMask == 0)
    return;

  const AVR32InstrInfo &TII =
      *MF.getSubtarget<AVR32Subtarget>().getInstrInfo();
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  if (PushmMask != 0)
    BuildMI(MBB, MBBI, DL, TII.get(AVR32::PUSHM))
        .addImm(PushmMask)
        .setMIFlag(MachineInstr::FrameSetup);

  emitSPAdjustment(MBB, MBBI, DL, TII, static_cast<int64_t>(StackSize),
                   MachineInstr::FrameSetup);

  if (HasFP)
    BuildMI(MBB, MBBI, DL, TII.get(AVR32::MOVrr), AVR32::R7)
        .addReg(AVR32::SP);
}

void AVR32FrameLowering::emitEpilogue(MachineFunction &MF,
                                      MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  uint64_t StackSize = MFI.getStackSize();
  bool HasFP = hasFP(MF);
  unsigned PushmMask = getPushmMask(MF);
  if (StackSize == 0 && !HasFP && PushmMask == 0)
    return;

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

  if (HasFP && (StackSize != 0 || MFI.hasVarSizedObjects()))
    BuildMI(MBB, MBBI, DL, TII.get(AVR32::MOVrr), AVR32::SP)
        .addReg(AVR32::R7);

  emitSPAdjustment(MBB, MBBI, DL, TII, -static_cast<int64_t>(StackSize),
                   MachineInstr::FrameDestroy);

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
