//===-- AVR32InstrInfo.cpp - AVR32 instruction information ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32InstrInfo.h"
#include "AVR32Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "AVR32GenInstrInfo.inc"
#define GET_INSTRINFO_CTOR_DTOR
#include "AVR32GenInstrInfo.inc"

AVR32InstrInfo::AVR32InstrInfo(const AVR32Subtarget &STI)
    : AVR32GenInstrInfo(STI, RI, AVR32::ADJCALLSTACKDOWN,
                        AVR32::ADJCALLSTACKUP),
      RI() {}

static bool isUnconditionalBranchOpcode(unsigned Opcode) {
  return Opcode == AVR32::BRALbb;
}

static bool isConditionalBranchOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVR32::BREQbb:
  case AVR32::BRNEbb:
  case AVR32::BRCCbb:
  case AVR32::BRCSbb:
  case AVR32::BRGEbb:
  case AVR32::BRLTbb:
  case AVR32::BRLSbb:
  case AVR32::BRGTbb:
  case AVR32::BRLEbb:
  case AVR32::BRHIbb:
    return true;
  default:
    return false;
  }
}

static bool isBranchOpcode(unsigned Opcode) {
  return isUnconditionalBranchOpcode(Opcode) ||
         isConditionalBranchOpcode(Opcode);
}

static unsigned getReversedBranchOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVR32::BREQbb:
    return AVR32::BRNEbb;
  case AVR32::BRNEbb:
    return AVR32::BREQbb;
  case AVR32::BRCCbb:
    return AVR32::BRCSbb;
  case AVR32::BRCSbb:
    return AVR32::BRCCbb;
  case AVR32::BRGEbb:
    return AVR32::BRLTbb;
  case AVR32::BRLTbb:
    return AVR32::BRGEbb;
  case AVR32::BRLSbb:
    return AVR32::BRHIbb;
  case AVR32::BRHIbb:
    return AVR32::BRLSbb;
  case AVR32::BRGTbb:
    return AVR32::BRLEbb;
  case AVR32::BRLEbb:
    return AVR32::BRGTbb;
  default:
    llvm_unreachable("Invalid AVR32 branch condition");
  }
}

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
  if (RC != &AVR32::GPRRegClass)
    llvm_unreachable("AVR32 can only store GPR registers to stack slots");

  DebugLoc DL;
  if (MI != MBB.end())
    DL = MI->getDebugLoc();

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOStore, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  BuildMI(MBB, MI, DL, get(AVR32::ST_W_Disp16))
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addReg(SrcReg, getKillRegState(isKill))
      .addMemOperand(MMO);
}

void AVR32InstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MI,
                                          Register DestReg, int FrameIdx,
                                          const TargetRegisterClass *RC,
                                          Register VReg, unsigned SubReg,
                                          MachineInstr::MIFlag Flags) const {
  if (RC != &AVR32::GPRRegClass)
    llvm_unreachable("AVR32 can only load GPR registers from stack slots");

  DebugLoc DL;
  if (MI != MBB.end())
    DL = MI->getDebugLoc();

  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIdx),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIdx),
      MFI.getObjectAlign(FrameIdx));

  BuildMI(MBB, MI, DL, get(AVR32::LD_W_Disp16), DestReg)
      .addFrameIndex(FrameIdx)
      .addImm(0)
      .addMemOperand(MMO);
}

bool AVR32InstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                   MachineBasicBlock *&TBB,
                                   MachineBasicBlock *&FBB,
                                   SmallVectorImpl<MachineOperand> &Cond,
                                   bool AllowModify) const {
  MachineBasicBlock::iterator I = MBB.end();
  while (I != MBB.begin()) {
    --I;
    if (I->isDebugInstr())
      continue;

    if (!isUnpredicatedTerminator(*I))
      break;

    if (!I->isBranch())
      return true;

    unsigned Opcode = I->getOpcode();
    if (!isBranchOpcode(Opcode))
      return true;

    if (isUnconditionalBranchOpcode(Opcode)) {
      if (!AllowModify) {
        TBB = I->getOperand(0).getMBB();
        continue;
      }

      MBB.erase(std::next(I), MBB.end());
      Cond.clear();
      FBB = nullptr;

      if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
        TBB = nullptr;
        I->eraseFromParent();
        I = MBB.end();
        continue;
      }

      TBB = I->getOperand(0).getMBB();
      continue;
    }

    if (Cond.empty()) {
      FBB = TBB;
      TBB = I->getOperand(0).getMBB();
      Cond.push_back(MachineOperand::CreateImm(Opcode));
      continue;
    }

    return true;
  }

  return false;
}

unsigned AVR32InstrInfo::removeBranch(MachineBasicBlock &MBB,
                                      int *BytesRemoved) const {
  MachineBasicBlock::iterator I = MBB.end();
  unsigned Count = 0;

  while (I != MBB.begin()) {
    --I;
    if (I->isDebugInstr())
      continue;
    if (!isBranchOpcode(I->getOpcode()))
      break;

    I->eraseFromParent();
    I = MBB.end();
    ++Count;
  }

  if (BytesRemoved)
    *BytesRemoved = Count * 4;
  return Count;
}

unsigned AVR32InstrInfo::insertBranch(MachineBasicBlock &MBB,
                                      MachineBasicBlock *TBB,
                                      MachineBasicBlock *FBB,
                                      ArrayRef<MachineOperand> Cond,
                                      const DebugLoc &DL,
                                      int *BytesAdded) const {
  assert(TBB && "insertBranch must not be told to insert a fallthrough");
  assert(Cond.size() <= 1 && "AVR32 branch conditions have one component");

  unsigned Count = 0;
  if (Cond.empty()) {
    assert(!FBB && "Unconditional branch with multiple successors");
    BuildMI(&MBB, DL, get(AVR32::BRALbb)).addMBB(TBB);
    Count = 1;
  } else {
    unsigned Opcode = Cond[0].getImm();
    assert(isConditionalBranchOpcode(Opcode) &&
           "Invalid AVR32 conditional branch opcode");
    BuildMI(&MBB, DL, get(Opcode)).addMBB(TBB);
    Count = 1;

    if (FBB) {
      BuildMI(&MBB, DL, get(AVR32::BRALbb)).addMBB(FBB);
      ++Count;
    }
  }

  if (BytesAdded)
    *BytesAdded = Count * 4;
  return Count;
}

bool AVR32InstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 1 && "AVR32 branch conditions have one component");
  Cond[0].setImm(getReversedBranchOpcode(Cond[0].getImm()));
  return false;
}
