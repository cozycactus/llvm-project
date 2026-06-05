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
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#define DEBUG_TYPE "avr32-peephole"

#define GET_INSTRINFO_ENUM
#include "AVR32GenInstrInfo.inc"
#define GET_REGINFO_ENUM
#include "AVR32GenRegisterInfo.inc"

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
  bool foldSPLoadDisp(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldSPStoreDisp(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldMovhOr(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldBitImmediate(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldNearBranches(MachineFunction &MF, const TargetInstrInfo &TII);
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

static bool isLowMaterializationOperand(const MachineOperand &MO) {
  if (MO.isImm())
    return true;
  return (MO.isGlobal() || MO.isSymbol()) &&
         MO.getTargetFlags() == AVR32II::MO_ABS_LO;
}

static bool isHighMaterializationOperand(const MachineOperand &MO) {
  if (MO.isImm())
    return true;
  return (MO.isGlobal() || MO.isSymbol()) &&
         MO.getTargetFlags() == AVR32II::MO_ABS_HI;
}

static bool isMatchingHiLoPair(const MachineOperand &Low,
                               const MachineOperand &High) {
  if (Low.isImm() && High.isImm())
    return true;
  if (Low.isImm() || High.isImm())
    return false;
  if (Low.getTargetFlags() != AVR32II::MO_ABS_LO ||
      High.getTargetFlags() != AVR32II::MO_ABS_HI ||
      Low.getOffset() != High.getOffset())
    return false;
  if (Low.isGlobal() && High.isGlobal())
    return Low.getGlobal() == High.getGlobal();
  if (Low.isSymbol() && High.isSymbol())
    return StringRef(Low.getSymbolName()) == High.getSymbolName();
  return false;
}

static bool isMovLowMaterialization(const MachineInstr &MI) {
  return isMovLowImmOpcode(MI.getOpcode()) && MI.getNumOperands() == 2 &&
         isRegOperand(MI.getOperand(0)) &&
         isLowMaterializationOperand(MI.getOperand(1));
}

static bool isMovhMaterialization(const MachineInstr &MI) {
  return MI.getOpcode() == AVR32::MOVHri && MI.getNumOperands() == 2 &&
         isRegOperand(MI.getOperand(0)) &&
         isHighMaterializationOperand(MI.getOperand(1));
}

static std::optional<unsigned> getSingleSetBit(uint32_t Value) {
  if (Value == 0 || (Value & (Value - 1)) != 0)
    return std::nullopt;
  return llvm::countr_zero(Value);
}

static std::optional<unsigned> getBitSetIndex(unsigned Opcode, int64_t Imm) {
  uint32_t Value = static_cast<uint32_t>(Imm);
  switch (Opcode) {
  case AVR32::ORrr:
    return getSingleSetBit(Value);
  case AVR32::ORLri:
    return getSingleSetBit(Value & 0xffff);
  case AVR32::ORHri:
    if (std::optional<unsigned> Bit = getSingleSetBit(Value & 0xffff))
      return *Bit + 16;
    return std::nullopt;
  default:
    return std::nullopt;
  }
}

static std::optional<unsigned> getBitClearIndex(unsigned Opcode, int64_t Imm) {
  uint32_t Value = static_cast<uint32_t>(Imm);
  switch (Opcode) {
  case AVR32::ANDrr:
    return getSingleSetBit(~Value);
  case AVR32::ANDLri:
    return getSingleSetBit((~Value) & 0xffff);
  case AVR32::ANDHri:
    if (std::optional<unsigned> Bit = getSingleSetBit((~Value) & 0xffff))
      return *Bit + 16;
    return std::nullopt;
  default:
    return std::nullopt;
  }
}

static unsigned getInstSize(const MachineInstr &MI) {
  if (MI.isDebugInstr())
    return 0;
  return MI.getDesc().getSize();
}

static std::optional<unsigned> getCompactBranchOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVR32::BREQbb:
    return AVR32::BREQcbb;
  case AVR32::BRNEbb:
    return AVR32::BRNEcbb;
  case AVR32::BRCCbb:
    return AVR32::BRCCcbb;
  case AVR32::BRCSbb:
    return AVR32::BRCScbb;
  case AVR32::BRGEbb:
    return AVR32::BRGEcbb;
  case AVR32::BRLTbb:
    return AVR32::BRLTcbb;
  default:
    return std::nullopt;
  }
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
  if (!isMovLowMaterialization(LowMI) || !isMovhMaterialization(HighMI) ||
      !isMatchingHiLoPair(LowMI.getOperand(1), HighMI.getOperand(1)))
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
      .add(LowMI.getOperand(1))
      .setMIFlags(LowMI.getFlags());
  BuildMI(MBB, LowMI, HighMI.getDebugLoc(), TII.get(AVR32::ORHri), DstReg)
      .add(HighMI.getOperand(1))
      .setMIFlags(HighMI.getFlags());

  LowMI.eraseFromParent();
  HighMI.eraseFromParent();
  MI.eraseFromParent();
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::foldBitImmediate(MachineInstr &MI,
                                     const TargetInstrInfo &TII) {
  unsigned Opcode = MI.getOpcode();
  std::optional<unsigned> Bit;
  unsigned CompactOpc = 0;
  if (Opcode == AVR32::ORLri || Opcode == AVR32::ORHri ||
      Opcode == AVR32::ANDLri || Opcode == AVR32::ANDHri) {
    if (MI.getNumOperands() != 2 || !isRegOperand(MI.getOperand(0)) ||
        !MI.getOperand(1).isImm())
      return false;

    Bit = getBitSetIndex(Opcode, MI.getOperand(1).getImm());
    CompactOpc = AVR32::SBRri;
    if (!Bit) {
      Bit = getBitClearIndex(Opcode, MI.getOperand(1).getImm());
      CompactOpc = AVR32::CBRri;
    }
    if (!Bit)
      return false;

    MachineBasicBlock &MBB = *MI.getParent();
    BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(CompactOpc))
        .addReg(MI.getOperand(0).getReg())
        .addImm(*Bit)
        .setMIFlags(MI.getFlags());
    MI.eraseFromParent();
    ++NumCompactForms;
    return true;
  }

  if (Opcode != AVR32::ORrr && Opcode != AVR32::ANDrr)
    return false;
  if (MI.getNumOperands() != 2 || !isRegOperand(MI.getOperand(0)) ||
      !isRegOperand(MI.getOperand(1)) || !MI.getOperand(1).isKill())
    return false;
  if (MI.getIterator() == MI.getParent()->begin())
    return false;

  MachineBasicBlock::iterator ImmIt = std::prev(MI.getIterator());
  MachineInstr &ImmMI = *ImmIt;
  if (!isMovLowImmOpcode(ImmMI.getOpcode()) || ImmMI.getNumOperands() != 2 ||
      !isRegOperand(ImmMI.getOperand(0)) || !ImmMI.getOperand(1).isImm())
    return false;

  Register DstReg = MI.getOperand(0).getReg();
  Register ImmReg = MI.getOperand(1).getReg();
  if (DstReg == ImmReg || ImmMI.getOperand(0).getReg() != ImmReg)
    return false;

  Bit = getBitSetIndex(Opcode, ImmMI.getOperand(1).getImm());
  CompactOpc = AVR32::SBRri;
  if (!Bit) {
    Bit = getBitClearIndex(Opcode, ImmMI.getOperand(1).getImm());
    CompactOpc = AVR32::CBRri;
  }
  if (!Bit)
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  BuildMI(MBB, ImmMI, MI.getDebugLoc(), TII.get(CompactOpc))
      .addReg(DstReg)
      .addImm(*Bit)
      .setMIFlags(MI.getFlags());
  ImmMI.eraseFromParent();
  MI.eraseFromParent();
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::foldNearBranches(MachineFunction &MF,
                                     const TargetInstrInfo &TII) {
  DenseMap<const MachineBasicBlock *, uint64_t> BlockOffsets;
  DenseMap<const MachineInstr *, uint64_t> InstrOffsets;
  uint64_t Offset = 0;
  for (const MachineBasicBlock &MBB : MF) {
    BlockOffsets[&MBB] = Offset;
    for (const MachineInstr &MI : MBB) {
      InstrOffsets[&MI] = Offset;
      Offset += getInstSize(MI);
    }
  }

  bool Changed = false;
  for (MachineBasicBlock &MBB : MF) {
    for (MachineBasicBlock::iterator I = MBB.begin(), E = MBB.end(); I != E;) {
      MachineInstr &MI = *I++;
      unsigned CompactOpc = 0;
      int64_t MinDisp = 0;
      int64_t MaxDisp = 0;

      if (MI.getOpcode() == AVR32::BRALbb) {
        CompactOpc = AVR32::RJMPbb;
        MinDisp = -1000;
        MaxDisp = 998;
      } else if (std::optional<unsigned> Opc =
                     getCompactBranchOpcode(MI.getOpcode())) {
        CompactOpc = *Opc;
        MinDisp = -248;
        MaxDisp = 246;
      } else {
        continue;
      }

      if (MI.getNumOperands() != 1 || !MI.getOperand(0).isMBB())
        continue;

      const MachineBasicBlock *Target = MI.getOperand(0).getMBB();
      int64_t BranchOffset = static_cast<int64_t>(InstrOffsets.lookup(&MI));
      int64_t TargetOffset = static_cast<int64_t>(BlockOffsets.lookup(Target));
      int64_t Disp = TargetOffset - BranchOffset;
      if (Disp < MinDisp || Disp > MaxDisp || (Disp & 1))
        continue;

      BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(CompactOpc))
          .addMBB(MI.getOperand(0).getMBB())
          .setMIFlags(MI.getFlags());
      MI.eraseFromParent();
      ++NumCompactForms;
      Changed = true;
    }
  }

  return Changed;
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

bool AVR32Peephole::foldSPLoadDisp(MachineInstr &MI,
                                   const TargetInstrInfo &TII) {
  if (MI.getNumOperands() != 3)
    return false;

  const MachineOperand &Dst = MI.getOperand(0);
  const MachineOperand &Base = MI.getOperand(1);
  const MachineOperand &Disp = MI.getOperand(2);
  if (!isRegOperand(Dst) || !isRegOperand(Base) || Base.getReg() != AVR32::SP ||
      !Disp.isImm() || !isCompactDisp(Disp.getImm(), 508, 4))
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineInstrBuilder MIB =
      BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVR32::LDDSP), Dst.getReg())
          .addReg(AVR32::SP, getKillRegState(Base.isKill()))
          .addImm(Disp.getImm());
  MIB.setMIFlags(MI.getFlags());
  MIB->copyImplicitOps(MF, MI);
  MIB.setMemRefs(MI.memoperands());
  MI.eraseFromParent();
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::foldSPStoreDisp(MachineInstr &MI,
                                    const TargetInstrInfo &TII) {
  if (MI.getNumOperands() != 3)
    return false;

  const MachineOperand &Base = MI.getOperand(0);
  const MachineOperand &Disp = MI.getOperand(1);
  const MachineOperand &Src = MI.getOperand(2);
  if (!isRegOperand(Base) || Base.getReg() != AVR32::SP || !Disp.isImm() ||
      !isRegOperand(Src) || !isCompactDisp(Disp.getImm(), 508, 4))
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineInstrBuilder MIB =
      BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVR32::STDSP))
          .addReg(AVR32::SP, getKillRegState(Base.isKill()))
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

  bool LocalChanged;
  do {
    LocalChanged = false;
    for (MachineBasicBlock &MBB : MF) {
      for (MachineBasicBlock::iterator I = MBB.begin(), E = MBB.end(); I != E;) {
        MachineInstr &MI = *I++;
        switch (MI.getOpcode()) {
        case AVR32::ADDALrrr:
          LocalChanged |= foldTwoAddressALU(MI, AVR32::ADDrr, true, TII);
          break;
        case AVR32::ANDALrrr:
          LocalChanged |= foldTwoAddressALU(MI, AVR32::ANDrr, true, TII);
          break;
        case AVR32::ANDrr:
          LocalChanged |= foldBitImmediate(MI, TII);
          break;
        case AVR32::ANDLri:
        case AVR32::ANDHri:
          LocalChanged |= foldBitImmediate(MI, TII);
          break;
        case AVR32::EORALrrr:
          LocalChanged |= foldTwoAddressALU(MI, AVR32::EORrr, true, TII);
          break;
        case AVR32::MULrrr:
          LocalChanged |= foldTwoAddressALU(MI, AVR32::MULrr, true, TII);
          break;
        case AVR32::MOVri21:
          LocalChanged |= foldSignedImmediate(MI, AVR32::MOVri8, 8, true, TII);
          break;
        case AVR32::CPri21:
          LocalChanged |= foldSignedImmediate(MI, AVR32::CPri6, 6, false, TII);
          break;
        case AVR32::ORALrrr:
          LocalChanged |= foldMovhOr(MI, TII) ||
                          foldTwoAddressALU(MI, AVR32::ORrr, true, TII);
          break;
        case AVR32::ORrr:
          LocalChanged |= foldMovhOr(MI, TII) || foldBitImmediate(MI, TII);
          break;
        case AVR32::ORLri:
        case AVR32::ORHri:
          LocalChanged |= foldBitImmediate(MI, TII);
          break;
        case AVR32::CP_Wri21:
          LocalChanged |=
              foldSignedImmediate(MI, AVR32::CP_Wri6, 6, false, TII);
          break;
        case AVR32::LD_SH_Disp16:
          LocalChanged |= foldLoadDisp(MI, AVR32::LD_SH_Disp3, 14, 2, TII);
          break;
        case AVR32::LD_UB_Disp16:
          LocalChanged |= foldLoadDisp(MI, AVR32::LD_UB_Disp3, 7, 1, TII);
          break;
        case AVR32::LD_UH_Disp16:
          LocalChanged |= foldLoadDisp(MI, AVR32::LD_UH_Disp3, 14, 2, TII);
          break;
        case AVR32::LD_W_Disp16:
          LocalChanged |= foldSPLoadDisp(MI, TII) ||
                          foldLoadDisp(MI, AVR32::LD_W_Disp5, 124, 4, TII);
          break;
        case AVR32::ST_B_Disp16:
          LocalChanged |= foldStoreDisp(MI, AVR32::ST_B_Disp3, 7, 1, TII);
          break;
        case AVR32::ST_H_Disp16:
          LocalChanged |= foldStoreDisp(MI, AVR32::ST_H_Disp3, 14, 2, TII);
          break;
        case AVR32::ST_W_Disp16:
          LocalChanged |= foldSPStoreDisp(MI, TII) ||
                          foldStoreDisp(MI, AVR32::ST_W_Disp4, 60, 4, TII);
          break;
        case AVR32::SUBALrrr:
          LocalChanged |= foldTwoAddressALU(MI, AVR32::SUBrr, false, TII);
          break;
        default:
          break;
        }
      }
    }
    Changed |= LocalChanged;
  } while (LocalChanged);

  while (foldNearBranches(MF, TII))
    Changed = true;

  return Changed;
}

FunctionPass *llvm::createAVR32PeepholePass() { return new AVR32Peephole(); }
