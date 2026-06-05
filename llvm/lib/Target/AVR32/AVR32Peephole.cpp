//===-- AVR32Peephole.cpp - AVR32 machine peephole optimizations ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32.h"
#include "AVR32InstrInfo.h"
#include "AVR32Subtarget.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

using namespace llvm;

#define DEBUG_TYPE "avr32-peephole"

#define GET_INSTRINFO_ENUM
#include "AVR32GenInstrInfo.inc"

STATISTIC(NumCompactForms, "Number of compact instruction forms selected");

namespace {

class AVR32Peephole : public MachineFunctionPass {
public:
  static char ID;

  AVR32Peephole() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override {
    return "AVR32 machine peephole optimizations";
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  bool foldTwoAddressALU(MachineInstr &MI, unsigned CompactOpc,
                         bool Commutable, const TargetInstrInfo &TII);
  bool foldSignedImmediate(MachineInstr &MI, unsigned CompactOpc,
                           unsigned Bits, bool HasDef,
                           const TargetInstrInfo &TII);
  bool foldLoadDisp(MachineInstr &MI, unsigned CompactOpc, unsigned MaxDisp,
                    unsigned Align, const TargetInstrInfo &TII);
  bool foldStoreDisp(MachineInstr &MI, unsigned CompactOpc, unsigned MaxDisp,
                     unsigned Align, const TargetInstrInfo &TII);
  bool foldMovhOr(MachineInstr &MI, const TargetInstrInfo &TII);
};

} // end anonymous namespace

char AVR32Peephole::ID = 0;

INITIALIZE_PASS(AVR32Peephole, DEBUG_TYPE, "AVR32 Peephole Optimizations",
                false, false)

static bool isRegOperand(const MachineOperand &MO) {
  return MO.isReg() && MO.getReg().isValid() && !MO.getReg().isVirtual();
}

static bool isSignedNBit(int64_t Value, unsigned Bits) {
  int64_t Min = -(int64_t(1) << (Bits - 1));
  int64_t Max = (int64_t(1) << (Bits - 1)) - 1;
  return Value >= Min && Value <= Max;
}

static bool isCompactDisp(int64_t Value, unsigned MaxDisp, unsigned Align) {
  return Value >= 0 && static_cast<uint64_t>(Value) <= MaxDisp &&
         Value % Align == 0;
}

static bool isMovLowImmOpcode(unsigned Opcode) {
  return Opcode == AVR32::MOVri8 || Opcode == AVR32::MOVri21;
}

static bool isMovLowImm(const MachineInstr &MI) {
  return isMovLowImmOpcode(MI.getOpcode()) && MI.getNumOperands() == 2 &&
         isRegOperand(MI.getOperand(0)) && MI.getOperand(1).isImm();
}

static bool isMovhImm(const MachineInstr &MI) {
  return MI.getOpcode() == AVR32::MOVHri && MI.getNumOperands() == 2 &&
         isRegOperand(MI.getOperand(0)) && MI.getOperand(1).isImm();
}

bool AVR32Peephole::foldTwoAddressALU(MachineInstr &MI, unsigned CompactOpc,
                                      bool Commutable,
                                      const TargetInstrInfo &TII) {
  if (MI.getNumOperands() != 3)
    return false;

  const MachineOperand &Dst = MI.getOperand(0);
  const MachineOperand &LHS = MI.getOperand(1);
  const MachineOperand &RHS = MI.getOperand(2);
  if (!isRegOperand(Dst) || !isRegOperand(LHS) || !isRegOperand(RHS))
    return false;

  Register DstReg = Dst.getReg();
  unsigned SrcIdx = 0;
  if (DstReg == LHS.getReg())
    SrcIdx = 2;
  else if (Commutable && DstReg == RHS.getReg())
    SrcIdx = 1;
  else
    return false;

  const MachineOperand &Src = MI.getOperand(SrcIdx);
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineInstrBuilder MIB =
      BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(CompactOpc), DstReg)
          .addReg(Src.getReg(), getKillRegState(Src.isKill()));
  MIB.setMIFlags(MI.getFlags());
  MIB->copyImplicitOps(MF, MI);
  MI.eraseFromParent();
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::foldMovhOr(MachineInstr &MI,
                               const TargetInstrInfo &TII) {
  if (MI.getIterator() == MI.getParent()->begin())
    return false;

  MachineBasicBlock::iterator HighIt = std::prev(MI.getIterator());
  if (HighIt == MI.getParent()->begin())
    return false;
  MachineBasicBlock::iterator LowIt = std::prev(HighIt);
  MachineInstr &LowMI = *LowIt;
  MachineInstr &HighMI = *HighIt;
  if (!isMovLowImm(LowMI) || !isMovhImm(HighMI))
    return false;

  Register DstReg;
  Register LowReg = LowMI.getOperand(0).getReg();
  Register HighReg = HighMI.getOperand(0).getReg();

  if (MI.getOpcode() == AVR32::ORALrrr) {
    if (MI.getNumOperands() != 3 || !isRegOperand(MI.getOperand(0)) ||
        !isRegOperand(MI.getOperand(1)) || !isRegOperand(MI.getOperand(2)))
      return false;

    Register Src0 = MI.getOperand(1).getReg();
    Register Src1 = MI.getOperand(2).getReg();
    if (!((Src0 == HighReg && Src1 == LowReg) ||
          (Src0 == LowReg && Src1 == HighReg)))
      return false;
    DstReg = MI.getOperand(0).getReg();
  } else if (MI.getOpcode() == AVR32::ORrr) {
    if (MI.getNumOperands() != 2 || !isRegOperand(MI.getOperand(0)) ||
        !isRegOperand(MI.getOperand(1)))
      return false;

    DstReg = MI.getOperand(0).getReg();
    if (DstReg != LowReg || MI.getOperand(1).getReg() != HighReg)
      return false;
  } else {
    return false;
  }

  MachineBasicBlock &MBB = *MI.getParent();
  BuildMI(MBB, LowMI, LowMI.getDebugLoc(), TII.get(LowMI.getOpcode()), DstReg)
      .addImm(LowMI.getOperand(1).getImm())
      .setMIFlags(LowMI.getFlags());
  BuildMI(MBB, LowMI, HighMI.getDebugLoc(), TII.get(AVR32::ORHri), DstReg)
      .addImm(HighMI.getOperand(1).getImm())
      .setMIFlags(HighMI.getFlags());

  LowMI.eraseFromParent();
  HighMI.eraseFromParent();
  MI.eraseFromParent();
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::foldLoadDisp(MachineInstr &MI, unsigned CompactOpc,
                                 unsigned MaxDisp, unsigned Align,
                                 const TargetInstrInfo &TII) {
  if (MI.getNumOperands() != 3)
    return false;

  const MachineOperand &Dst = MI.getOperand(0);
  const MachineOperand &Base = MI.getOperand(1);
  const MachineOperand &Disp = MI.getOperand(2);
  if (!isRegOperand(Dst) || !isRegOperand(Base) || !Disp.isImm() ||
      !isCompactDisp(Disp.getImm(), MaxDisp, Align))
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineInstrBuilder MIB =
      BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(CompactOpc), Dst.getReg())
          .addReg(Base.getReg(), getKillRegState(Base.isKill()))
          .addImm(Disp.getImm());
  MIB.setMIFlags(MI.getFlags());
  MIB->copyImplicitOps(MF, MI);
  MIB.setMemRefs(MI.memoperands());
  MI.eraseFromParent();
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::foldStoreDisp(MachineInstr &MI, unsigned CompactOpc,
                                  unsigned MaxDisp, unsigned Align,
                                  const TargetInstrInfo &TII) {
  if (MI.getNumOperands() != 3)
    return false;

  const MachineOperand &Base = MI.getOperand(0);
  const MachineOperand &Disp = MI.getOperand(1);
  const MachineOperand &Src = MI.getOperand(2);
  if (!isRegOperand(Base) || !Disp.isImm() || !isRegOperand(Src) ||
      !isCompactDisp(Disp.getImm(), MaxDisp, Align))
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineInstrBuilder MIB =
      BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(CompactOpc))
          .addReg(Base.getReg(), getKillRegState(Base.isKill()))
          .addImm(Disp.getImm())
          .addReg(Src.getReg(), getKillRegState(Src.isKill()));
  MIB.setMIFlags(MI.getFlags());
  MIB->copyImplicitOps(MF, MI);
  MIB.setMemRefs(MI.memoperands());
  MI.eraseFromParent();
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::foldSignedImmediate(MachineInstr &MI, unsigned CompactOpc,
                                        unsigned Bits, bool HasDef,
                                        const TargetInstrInfo &TII) {
  if (MI.getNumOperands() != 2)
    return false;

  const MachineOperand &RegOp = MI.getOperand(0);
  const MachineOperand &ImmOp = MI.getOperand(1);
  if (!isRegOperand(RegOp) || !ImmOp.isImm() ||
      !isSignedNBit(ImmOp.getImm(), Bits))
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineInstrBuilder MIB =
      HasDef ? BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(CompactOpc),
                       RegOp.getReg())
             : BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(CompactOpc));
  if (!HasDef)
    MIB.addReg(RegOp.getReg(), getKillRegState(RegOp.isKill()));
  MIB.addImm(ImmOp.getImm());
  MIB.setMIFlags(MI.getFlags());
  MIB->copyImplicitOps(MF, MI);
  MI.eraseFromParent();
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::runOnMachineFunction(MachineFunction &MF) {
  const TargetInstrInfo &TII =
      *MF.getSubtarget<AVR32Subtarget>().getInstrInfo();
  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    for (MachineBasicBlock::iterator I = MBB.begin(), E = MBB.end(); I != E;) {
      MachineInstr &MI = *I++;
      switch (MI.getOpcode()) {
      case AVR32::ADDALrrr:
        Changed |= foldTwoAddressALU(MI, AVR32::ADDrr, true, TII);
        break;
      case AVR32::ANDALrrr:
        Changed |= foldTwoAddressALU(MI, AVR32::ANDrr, true, TII);
        break;
      case AVR32::EORALrrr:
        Changed |= foldTwoAddressALU(MI, AVR32::EORrr, true, TII);
        break;
      case AVR32::MULrrr:
        Changed |= foldTwoAddressALU(MI, AVR32::MULrr, true, TII);
        break;
      case AVR32::MOVri21:
        Changed |= foldSignedImmediate(MI, AVR32::MOVri8, 8, true, TII);
        break;
      case AVR32::CPri21:
        Changed |= foldSignedImmediate(MI, AVR32::CPri6, 6, false, TII);
        break;
      case AVR32::ORALrrr:
        Changed |= foldMovhOr(MI, TII) ||
                   foldTwoAddressALU(MI, AVR32::ORrr, true, TII);
        break;
      case AVR32::ORrr:
        Changed |= foldMovhOr(MI, TII);
        break;
      case AVR32::CP_Wri21:
        Changed |= foldSignedImmediate(MI, AVR32::CP_Wri6, 6, false, TII);
        break;
      case AVR32::LD_SH_Disp16:
        Changed |= foldLoadDisp(MI, AVR32::LD_SH_Disp3, 14, 2, TII);
        break;
      case AVR32::LD_UB_Disp16:
        Changed |= foldLoadDisp(MI, AVR32::LD_UB_Disp3, 7, 1, TII);
        break;
      case AVR32::LD_UH_Disp16:
        Changed |= foldLoadDisp(MI, AVR32::LD_UH_Disp3, 14, 2, TII);
        break;
      case AVR32::LD_W_Disp16:
        Changed |= foldLoadDisp(MI, AVR32::LD_W_Disp5, 124, 4, TII);
        break;
      case AVR32::ST_B_Disp16:
        Changed |= foldStoreDisp(MI, AVR32::ST_B_Disp3, 7, 1, TII);
        break;
      case AVR32::ST_H_Disp16:
        Changed |= foldStoreDisp(MI, AVR32::ST_H_Disp3, 14, 2, TII);
        break;
      case AVR32::ST_W_Disp16:
        Changed |= foldStoreDisp(MI, AVR32::ST_W_Disp4, 60, 4, TII);
        break;
      case AVR32::SUBALrrr:
        Changed |= foldTwoAddressALU(MI, AVR32::SUBrr, false, TII);
        break;
      default:
        break;
      }
    }
  }

  return Changed;
}

FunctionPass *llvm::createAVR32PeepholePass() { return new AVR32Peephole(); }
