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
        Changed |= foldTwoAddressALU(MI, AVR32::ORrr, true, TII);
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
