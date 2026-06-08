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
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
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
  bool foldCompareImmediate(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldDoubleCompareLibcall(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldShiftedMaskIndexLoad(MachineInstr &MI, const TargetInstrInfo &TII,
                                const TargetRegisterInfo &TRI);
  bool foldNarrowCompare(MachineInstr &MI, const TargetInstrInfo &TII,
                         const TargetRegisterInfo &TRI);
  bool foldRedundantCastBeforeCompare(MachineInstr &MI,
                                      const TargetRegisterInfo &TRI);
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

static bool isMoveImmToReg(const MachineInstr &MI, Register Reg,
                           int64_t Imm) {
  return isMovLowImmOpcode(MI.getOpcode()) && MI.getNumOperands() >= 2 &&
         isRegOperand(MI.getOperand(0)) && MI.getOperand(0).getReg() == Reg &&
         MI.getOperand(1).isImm() && MI.getOperand(1).getImm() == Imm;
}

static MachineBasicBlock::iterator
prevNonDebug(MachineBasicBlock::iterator I) {
  MachineBasicBlock &MBB = *I->getParent();
  while (I != MBB.begin()) {
    --I;
    if (!I->isDebugInstr())
      return I;
  }
  return MBB.end();
}

static StringRef getDirectCallTargetName(const MachineInstr &MI) {
  if (MI.getOpcode() != AVR32::RCALLeCG || MI.getNumOperands() == 0)
    return StringRef();

  const MachineOperand &Target = MI.getOperand(0);
  if (Target.isSymbol())
    return Target.getSymbolName();
  if (Target.isGlobal())
    return Target.getGlobal()->getName();
  return StringRef();
}

static bool retargetDirectCall(MachineInstr &MI, StringRef From,
                               const char *To) {
  if (getDirectCallTargetName(MI) != From)
    return false;

  MachineOperand &Target = MI.getOperand(0);
  Target.ChangeToES(To, Target.getTargetFlags());
  return true;
}

static void insertF64ArgSwapBefore(MachineInstr &Call,
                                   const TargetInstrInfo &TII) {
  MachineBasicBlock &MBB = *Call.getParent();
  const DebugLoc &DL = Call.getDebugLoc();
  BuildMI(MBB, Call, DL, TII.get(AVR32::MOVrr), AVR32::R12).addReg(AVR32::R9);
  BuildMI(MBB, Call, DL, TII.get(AVR32::MOVrr), AVR32::LR).addReg(AVR32::R8);
  BuildMI(MBB, Call, DL, TII.get(AVR32::MOVrr), AVR32::R9).addReg(AVR32::R11);
  BuildMI(MBB, Call, DL, TII.get(AVR32::MOVrr), AVR32::R8).addReg(AVR32::R10);
  BuildMI(MBB, Call, DL, TII.get(AVR32::MOVrr), AVR32::R11)
      .addReg(AVR32::R12);
  BuildMI(MBB, Call, DL, TII.get(AVR32::MOVrr), AVR32::R10).addReg(AVR32::LR);
}

static bool isCompareOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVR32::CPrr:
  case AVR32::CP_Wrr:
  case AVR32::CPri6:
  case AVR32::CPri21:
  case AVR32::CP_Wri6:
  case AVR32::CP_Wri21:
  case AVR32::CP_Brr:
  case AVR32::CP_Hrr:
    return true;
  default:
    return false;
  }
}

static bool compareUsesReg(const MachineInstr &MI, Register Reg) {
  if (!isCompareOpcode(MI.getOpcode()))
    return false;
  for (const MachineOperand &MO : MI.operands())
    if (MO.isReg() && MO.isUse() && MO.getReg() == Reg)
      return true;
  return false;
}

static std::optional<unsigned> getCompareImmOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVR32::CPrr:
    return AVR32::CPri21;
  case AVR32::CP_Wrr:
    return AVR32::CP_Wri21;
  default:
    return std::nullopt;
  }
}

static bool isImmediateCompareOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVR32::CPri6:
  case AVR32::CPri21:
  case AVR32::CP_Wri6:
  case AVR32::CP_Wri21:
    return true;
  default:
    return false;
  }
}

static bool explicitlyDefinesReg(const MachineInstr &MI, Register Reg) {
  for (const MachineOperand &MO : MI.operands())
    if (MO.isReg() && MO.isDef() && MO.getReg() == Reg)
      return true;
  return false;
}

static bool isEqualityConditionOpcode(unsigned Opcode) {
  return Opcode == AVR32::BREQbb || Opcode == AVR32::BRNEbb ||
         Opcode == AVR32::MOVEQriCG || Opcode == AVR32::MOVNEriCG ||
         Opcode == AVR32::MOVEQrrCG || Opcode == AVR32::MOVNErrCG;
}

static bool isUnsignedConditionOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVR32::BRCCbb:
  case AVR32::BRCSbb:
  case AVR32::BRLSbb:
  case AVR32::BRHIbb:
  case AVR32::MOVCCriCG:
  case AVR32::MOVCSriCG:
  case AVR32::MOVLSriCG:
  case AVR32::MOVHIriCG:
  case AVR32::MOVCCrrCG:
  case AVR32::MOVCSrrCG:
  case AVR32::MOVLSrrCG:
  case AVR32::MOVHIrrCG:
    return true;
  default:
    return false;
  }
}

static bool isSignedConditionOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVR32::BRGEbb:
  case AVR32::BRLTbb:
  case AVR32::BRGTbb:
  case AVR32::BRLEbb:
  case AVR32::MOVGEriCG:
  case AVR32::MOVLTriCG:
  case AVR32::MOVGTriCG:
  case AVR32::MOVLEriCG:
  case AVR32::MOVGErrCG:
  case AVR32::MOVLTrrCG:
  case AVR32::MOVGTrrCG:
  case AVR32::MOVLErrCG:
    return true;
  default:
    return false;
  }
}

static bool hasCompatibleNarrowCompareUser(const MachineInstr &MI,
                                           bool Signed) {
  MachineBasicBlock::const_iterator NextIt = std::next(MI.getIterator());
  const MachineBasicBlock &MBB = *MI.getParent();
  while (NextIt != MBB.end() && NextIt->isDebugInstr())
    ++NextIt;
  if (NextIt == MBB.end())
    return false;

  unsigned Opcode = NextIt->getOpcode();
  if (isEqualityConditionOpcode(Opcode))
    return true;
  return Signed ? isSignedConditionOpcode(Opcode)
                : isUnsignedConditionOpcode(Opcode);
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

static std::optional<unsigned> getZeroExtendedWidth(const MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case AVR32::LD_UB_Disp3:
  case AVR32::LD_UB_Disp16:
  case AVR32::LD_UB_IndexShift:
  case AVR32::LD_UB_PostInc:
  case AVR32::LD_UB_PreDec:
  case AVR32::CASTU_Bcg:
    return 8;
  case AVR32::LD_UH_Disp3:
  case AVR32::LD_UH_Disp16:
  case AVR32::LD_UH_IndexShift:
  case AVR32::LD_UH_PostInc:
  case AVR32::LD_UH_PreDec:
  case AVR32::CASTU_Hcg:
    return 16;
  default:
    return std::nullopt;
  }
}

static std::optional<unsigned> getSignExtendedWidth(const MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case AVR32::LD_SB_Disp16:
  case AVR32::LD_SB_IndexShift:
  case AVR32::CASTS_Bcg:
    return 8;
  case AVR32::LD_SH_Disp3:
  case AVR32::LD_SH_Disp16:
  case AVR32::LD_SH_IndexShift:
  case AVR32::LD_SH_PostInc:
  case AVR32::LD_SH_PreDec:
  case AVR32::CASTS_Hcg:
    return 16;
  default:
    return std::nullopt;
  }
}

static bool isImmediateAlreadyExtended(int64_t Imm, bool Signed,
                                       unsigned Width) {
  if (Signed)
    return isSignedNBit(Imm, Width);
  return Imm >= 0 && static_cast<uint64_t>(Imm) < (uint64_t(1) << Width);
}

static bool isRegAlreadyExtendedBefore(MachineBasicBlock::iterator Pos,
                                       Register Reg, bool Signed,
                                       unsigned Width,
                                       const TargetRegisterInfo &TRI,
                                       unsigned Depth = 8) {
  if (!Depth)
    return false;

  MachineBasicBlock &MBB = *Pos->getParent();
  for (MachineBasicBlock::iterator I = Pos; I != MBB.begin();) {
    --I;
    MachineInstr &DefMI = *I;
    if (DefMI.isDebugInstr())
      continue;
    if (!DefMI.modifiesRegister(Reg, &TRI))
      continue;
    if (!explicitlyDefinesReg(DefMI, Reg))
      return false;

    if (DefMI.getOpcode() == AVR32::MOVrr && DefMI.getNumOperands() == 2 &&
        isRegOperand(DefMI.getOperand(0)) && isRegOperand(DefMI.getOperand(1)) &&
        DefMI.getOperand(0).getReg() == Reg)
      return isRegAlreadyExtendedBefore(I, DefMI.getOperand(1).getReg(),
                                        Signed, Width, TRI, Depth - 1);

    if (isMovLowImmOpcode(DefMI.getOpcode()) && DefMI.getNumOperands() == 2 &&
        isRegOperand(DefMI.getOperand(0)) && DefMI.getOperand(0).getReg() == Reg &&
        DefMI.getOperand(1).isImm())
      return isImmediateAlreadyExtended(DefMI.getOperand(1).getImm(), Signed,
                                        Width);

    std::optional<unsigned> KnownWidth =
        Signed ? getSignExtendedWidth(DefMI) : getZeroExtendedWidth(DefMI);
    if (KnownWidth && *KnownWidth <= Width)
      return true;

    return false;
  }

  return false;
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

bool AVR32Peephole::foldCompareImmediate(MachineInstr &MI,
                                         const TargetInstrInfo &TII) {
  std::optional<unsigned> ImmOpc = getCompareImmOpcode(MI.getOpcode());
  if (!ImmOpc || MI.getNumOperands() != 2 || !isRegOperand(MI.getOperand(0)) ||
      !isRegOperand(MI.getOperand(1)) || !MI.getOperand(1).isKill())
    return false;
  if (MI.getIterator() == MI.getParent()->begin())
    return false;

  MachineBasicBlock::iterator ImmIt = std::prev(MI.getIterator());
  MachineInstr &ImmMI = *ImmIt;
  if (!isMovLowImmOpcode(ImmMI.getOpcode()) || ImmMI.getNumOperands() != 2 ||
      !isRegOperand(ImmMI.getOperand(0)) || !ImmMI.getOperand(1).isImm() ||
      ImmMI.getOperand(0).getReg() != MI.getOperand(1).getReg())
    return false;

  int64_t Imm = ImmMI.getOperand(1).getImm();
  if (!isSignedNBit(Imm, 21))
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineInstrBuilder MIB =
      BuildMI(MBB, ImmMI, MI.getDebugLoc(), TII.get(*ImmOpc))
          .addReg(MI.getOperand(0).getReg(),
                  getKillRegState(MI.getOperand(0).isKill()))
          .addImm(Imm);
  MIB.setMIFlags(MI.getFlags());
  MIB->copyImplicitOps(MF, MI);
  ImmMI.eraseFromParent();
  MI.eraseFromParent();
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::foldDoubleCompareLibcall(
    MachineInstr &MI, const TargetInstrInfo &TII) {
  if ((MI.getOpcode() != AVR32::MOVEQriCG &&
       MI.getOpcode() != AVR32::MOVLTriCG) ||
      MI.getNumOperands() < 3 || !isRegOperand(MI.getOperand(0)) ||
      !isRegOperand(MI.getOperand(1)) ||
      MI.getOperand(0).getReg() != MI.getOperand(1).getReg() ||
      !MI.getOperand(2).isImm() || MI.getOperand(2).getImm() != 1)
    return false;

  Register BoolReg = MI.getOperand(0).getReg();
  MachineBasicBlock &MBB = *MI.getParent();

  MachineBasicBlock::iterator Prev = prevNonDebug(MI.getIterator());
  if (Prev == MBB.end())
    return false;

  if (Prev->getOpcode() == AVR32::CPrr && Prev->getNumOperands() >= 2 &&
      isRegOperand(Prev->getOperand(0)) &&
      isRegOperand(Prev->getOperand(1))) {
    bool IsLTOrEQ = Prev->getOperand(0).getReg() == AVR32::R12 &&
                    Prev->getOperand(1).getReg() == BoolReg;
    bool IsGT = MI.getOpcode() == AVR32::MOVLTriCG &&
                Prev->getOperand(0).getReg() == BoolReg &&
                Prev->getOperand(1).getReg() == AVR32::R12;
    if (!IsLTOrEQ && !IsGT)
      return false;

    MachineBasicBlock::iterator Zero = prevNonDebug(Prev);
    if (Zero == MBB.end() || !isMoveImmToReg(*Zero, BoolReg, 0))
      return false;

    MachineBasicBlock::iterator Call = prevNonDebug(Zero);
    if (Call == MBB.end())
      return false;

    StringRef From;
    const char *To = nullptr;
    if (IsGT) {
      From = "__gtdf2";
      To = "__avr32_f64_cmp_lt";
    } else if (MI.getOpcode() == AVR32::MOVEQriCG) {
      From = "__eqdf2";
      To = "__avr32_f64_cmp_eq";
    } else {
      From = "__ltdf2";
      To = "__avr32_f64_cmp_lt";
    }

    if (!retargetDirectCall(*Call, From, To))
      return false;

    if (IsGT)
      insertF64ArgSwapBefore(*Call, TII);
    Prev->eraseFromParent();
    MI.setDesc(TII.get(AVR32::MOVNEriCG));
    ++NumCompactForms;
    return true;
  }

  if (MI.getOpcode() != AVR32::MOVLTriCG ||
      !isMoveImmToReg(*Prev, BoolReg, 0))
    return false;

  MachineBasicBlock::iterator Cmp = prevNonDebug(Prev);
  if (Cmp == MBB.end())
    return false;

  if (isImmediateCompareOpcode(Cmp->getOpcode()) &&
      Cmp->getNumOperands() >= 2 && isRegOperand(Cmp->getOperand(0)) &&
      Cmp->getOperand(0).getReg() == AVR32::R12 &&
      Cmp->getOperand(1).isImm() && Cmp->getOperand(1).getImm() == 1) {
    MachineBasicBlock::iterator Call = prevNonDebug(Cmp);
    if (Call == MBB.end() ||
        !retargetDirectCall(*Call, "__ledf2", "__avr32_f64_cmp_ge"))
      return false;

    insertF64ArgSwapBefore(*Call, TII);
    Cmp->eraseFromParent();
    MI.setDesc(TII.get(AVR32::MOVNEriCG));
    ++NumCompactForms;
    return true;
  }

  if (Cmp->getOpcode() != AVR32::CPrr || Cmp->getNumOperands() < 2 ||
      !isRegOperand(Cmp->getOperand(0)) || !isRegOperand(Cmp->getOperand(1)) ||
      Cmp->getOperand(1).getReg() != AVR32::R12)
    return false;

  Register MinusOneReg = Cmp->getOperand(0).getReg();
  MachineBasicBlock::iterator MinusOne = prevNonDebug(Cmp);
  if (MinusOne == MBB.end() ||
      !isMoveImmToReg(*MinusOne, MinusOneReg, -1))
    return false;

  MachineBasicBlock::iterator Call = prevNonDebug(MinusOne);
  if (Call == MBB.end() ||
      !retargetDirectCall(*Call, "__gedf2", "__avr32_f64_cmp_ge"))
    return false;

  MinusOne->eraseFromParent();
  Cmp->eraseFromParent();
  MI.setDesc(TII.get(AVR32::MOVNEriCG));
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::foldShiftedMaskIndexLoad(
    MachineInstr &MI, const TargetInstrInfo &TII,
    const TargetRegisterInfo &TRI) {
  if (MI.getOpcode() != AVR32::LD_W_IndexShift || MI.getNumOperands() != 4 ||
      !isRegOperand(MI.getOperand(0)) || !isRegOperand(MI.getOperand(2)) ||
      !MI.getOperand(3).isImm())
    return false;

  Register IndexReg = MI.getOperand(2).getReg();
  if (MI.getOperand(0).getReg() != IndexReg && !MI.getOperand(2).isKill())
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  MachineBasicBlock::iterator AndIt = MBB.end();
  MachineBasicBlock::iterator I = MI.getIterator();
  for (unsigned Depth = 0; I != MBB.begin() && Depth < 4;) {
    --I;
    if (I->isDebugInstr())
      continue;
    if (I->getOpcode() == AVR32::ANDrr && I->getNumOperands() == 2 &&
        isRegOperand(I->getOperand(0)) &&
        I->getOperand(0).getReg() == IndexReg) {
      AndIt = I;
      break;
    }
    if (I->readsRegister(IndexReg, &TRI) || I->modifiesRegister(IndexReg, &TRI))
      return false;
    ++Depth;
  }
  if (AndIt == MBB.end() || !isRegOperand(AndIt->getOperand(1)) ||
      !AndIt->getOperand(1).isKill())
    return false;

  Register MaskReg = AndIt->getOperand(1).getReg();
  if (MaskReg == IndexReg)
    return false;

  MachineBasicBlock::iterator MaskIt = prevNonDebug(AndIt);
  if (MaskIt == MBB.end() || !isMovLowImmOpcode(MaskIt->getOpcode()) ||
      MaskIt->getNumOperands() != 2 || !isRegOperand(MaskIt->getOperand(0)) ||
      !MaskIt->getOperand(1).isImm() ||
      MaskIt->getOperand(0).getReg() != MaskReg)
    return false;

  MachineBasicBlock::iterator LsrIt = prevNonDebug(MaskIt);
  if (LsrIt == MBB.end() || LsrIt->getOpcode() != AVR32::LSRrrr ||
      LsrIt->getNumOperands() != 3 || !isRegOperand(LsrIt->getOperand(0)) ||
      !isRegOperand(LsrIt->getOperand(1)) ||
      !isRegOperand(LsrIt->getOperand(2)) ||
      LsrIt->getOperand(0).getReg() != IndexReg)
    return false;

  Register SrcReg = LsrIt->getOperand(1).getReg();
  Register ShiftReg = LsrIt->getOperand(2).getReg();
  if (ShiftReg == SrcReg)
    return false;

  MachineBasicBlock::iterator ShiftIt = prevNonDebug(LsrIt);
  if (ShiftIt == MBB.end() || !isMovLowImmOpcode(ShiftIt->getOpcode()) ||
      ShiftIt->getNumOperands() != 2 ||
      !isRegOperand(ShiftIt->getOperand(0)) ||
      !ShiftIt->getOperand(1).isImm() ||
      ShiftIt->getOperand(0).getReg() != ShiftReg)
    return false;

  int64_t ShiftImm = ShiftIt->getOperand(1).getImm();
  if (ShiftImm < 0 || ShiftImm > 31)
    return false;

  unsigned MaskIdx = 0;
  unsigned MaskLen = 0;
  uint32_t Mask = static_cast<uint32_t>(MaskIt->getOperand(1).getImm());
  if (!isShiftedMask_32(Mask, MaskIdx, MaskLen) || Mask == UINT32_MAX)
    return false;

  int64_t OldLoadShift = MI.getOperand(3).getImm();
  if (OldLoadShift < 0 || OldLoadShift > 3)
    return false;

  unsigned NewBitPos = static_cast<unsigned>(ShiftImm) + MaskIdx;
  unsigned NewLoadShift = static_cast<unsigned>(OldLoadShift) + MaskIdx;
  if (NewBitPos > 31 || NewLoadShift > 3)
    return false;

  MachineFunction &MF = *MBB.getParent();
  MachineInstrBuilder Extract =
      BuildMI(MBB, ShiftIt, LsrIt->getDebugLoc(), TII.get(AVR32::BFEXTU),
              IndexReg)
          .addReg(SrcReg, getKillRegState(LsrIt->getOperand(1).isKill()))
          .addImm(NewBitPos)
          .addImm(MaskLen);
  Extract.setMIFlags(LsrIt->getFlags());
  Extract->copyImplicitOps(MF, *LsrIt);

  MI.getOperand(3).setImm(NewLoadShift);
  ShiftIt->eraseFromParent();
  LsrIt->eraseFromParent();
  MaskIt->eraseFromParent();
  AndIt->eraseFromParent();
  ++NumCompactForms;
  return true;
}

static std::optional<unsigned> getCastWidth(unsigned Opcode, bool &Signed) {
  Signed = false;
  switch (Opcode) {
  case AVR32::CASTU_Bcg:
    return 8;
  case AVR32::CASTU_Hcg:
    return 16;
  case AVR32::CASTS_Bcg:
    Signed = true;
    return 8;
  case AVR32::CASTS_Hcg:
    Signed = true;
    return 16;
  default:
    return std::nullopt;
  }
}

static unsigned getNarrowCompareOpcode(unsigned Width) {
  switch (Width) {
  case 8:
    return AVR32::CP_Brr;
  case 16:
    return AVR32::CP_Hrr;
  default:
    return 0;
  }
}

static bool immediateFitsNarrowCompare(int64_t Imm, bool Signed,
                                       unsigned Width) {
  if (Signed)
    return isSignedNBit(Imm, Width);
  return Imm >= 0 && static_cast<uint64_t>(Imm) < (uint64_t(1) << Width);
}

static unsigned getMoveImmediateOpcode(int64_t Imm) {
  return isSignedNBit(Imm, 8) ? AVR32::MOVri8 : AVR32::MOVri21;
}

bool AVR32Peephole::foldNarrowCompare(MachineInstr &MI,
                                      const TargetInstrInfo &TII,
                                      const TargetRegisterInfo &TRI) {
  if (!isCompareOpcode(MI.getOpcode()) || MI.getNumOperands() != 2 ||
      !isRegOperand(MI.getOperand(0)) || !MI.getOperand(0).isKill())
    return false;
  if (!hasCompatibleNarrowCompareUser(MI, /*Signed=*/false) &&
      !hasCompatibleNarrowCompareUser(MI, /*Signed=*/true))
    return false;

  MachineBasicBlock::iterator CastIt = MI.getIterator();
  if (CastIt == MI.getParent()->begin())
    return false;
  --CastIt;
  while (CastIt->isDebugInstr()) {
    if (CastIt == MI.getParent()->begin())
      return false;
    --CastIt;
  }

  bool Signed = false;
  std::optional<unsigned> Width = getCastWidth(CastIt->getOpcode(), Signed);
  if (!Width || !hasCompatibleNarrowCompareUser(MI, Signed))
    return false;
  unsigned NarrowOpc = getNarrowCompareOpcode(*Width);
  if (!NarrowOpc || CastIt->getNumOperands() != 2 ||
      !isRegOperand(CastIt->getOperand(0)) ||
      !isRegOperand(CastIt->getOperand(1)))
    return false;

  Register CastReg = CastIt->getOperand(0).getReg();
  if (CastReg != CastIt->getOperand(1).getReg() ||
      CastReg != MI.getOperand(0).getReg())
    return false;

  MachineBasicBlock::iterator CopyIt = CastIt;
  if (CopyIt == MI.getParent()->begin())
    return false;
  --CopyIt;
  while (CopyIt->isDebugInstr()) {
    if (CopyIt == MI.getParent()->begin())
      return false;
    --CopyIt;
  }
  if (CopyIt->getOpcode() != AVR32::MOVrr || CopyIt->getNumOperands() != 2 ||
      !isRegOperand(CopyIt->getOperand(0)) ||
      !isRegOperand(CopyIt->getOperand(1)) ||
      CopyIt->getOperand(0).getReg() != CastReg)
    return false;

  Register SrcReg = CopyIt->getOperand(1).getReg();
  if (SrcReg == CastReg)
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  unsigned OldSize = getInstSize(*CopyIt) + getInstSize(*CastIt) +
                     getInstSize(MI);

  if (isImmediateCompareOpcode(MI.getOpcode())) {
    if (!MI.getOperand(1).isImm())
      return false;
    int64_t Imm = MI.getOperand(1).getImm();
    if (!immediateFitsNarrowCompare(Imm, Signed, *Width))
      return false;

    unsigned MovOpc = getMoveImmediateOpcode(Imm);
    unsigned NewSize = TII.get(MovOpc).getSize() + TII.get(NarrowOpc).getSize();
    if (NewSize >= OldSize)
      return false;

    BuildMI(MBB, CopyIt, CopyIt->getDebugLoc(), TII.get(MovOpc), CastReg)
        .addImm(Imm)
        .setMIFlags(CopyIt->getFlags());
    MachineInstrBuilder Cmp =
        BuildMI(MBB, CopyIt, MI.getDebugLoc(), TII.get(NarrowOpc))
            .addReg(SrcReg, getKillRegState(CopyIt->getOperand(1).isKill()))
            .addReg(CastReg, RegState::Kill);
    Cmp.setMIFlags(MI.getFlags());
    Cmp->copyImplicitOps(MF, MI);
  } else {
    if (!isRegOperand(MI.getOperand(1)))
      return false;
    Register RHSReg = MI.getOperand(1).getReg();
    if (!isRegAlreadyExtendedBefore(MI.getIterator(), RHSReg, Signed, *Width,
                                    TRI))
      return false;

    unsigned NewSize = TII.get(NarrowOpc).getSize();
    if (NewSize >= OldSize)
      return false;

    MachineInstrBuilder Cmp =
        BuildMI(MBB, CopyIt, MI.getDebugLoc(), TII.get(NarrowOpc))
            .addReg(SrcReg, getKillRegState(CopyIt->getOperand(1).isKill()))
            .addReg(RHSReg, getKillRegState(MI.getOperand(1).isKill()));
    Cmp.setMIFlags(MI.getFlags());
    Cmp->copyImplicitOps(MF, MI);
  }

  CopyIt->eraseFromParent();
  CastIt->eraseFromParent();
  MI.eraseFromParent();
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::foldRedundantCastBeforeCompare(
    MachineInstr &MI, const TargetRegisterInfo &TRI) {
  bool Signed = false;
  unsigned Width = 0;
  switch (MI.getOpcode()) {
  case AVR32::CASTU_Bcg:
    Width = 8;
    break;
  case AVR32::CASTU_Hcg:
    Width = 16;
    break;
  case AVR32::CASTS_Bcg:
    Signed = true;
    Width = 8;
    break;
  case AVR32::CASTS_Hcg:
    Signed = true;
    Width = 16;
    break;
  default:
    return false;
  }

  if (MI.getNumOperands() != 2 || !isRegOperand(MI.getOperand(0)) ||
      !isRegOperand(MI.getOperand(1)) ||
      MI.getOperand(0).getReg() != MI.getOperand(1).getReg())
    return false;

  MachineBasicBlock::iterator NextIt = std::next(MI.getIterator());
  MachineBasicBlock &MBB = *MI.getParent();
  while (NextIt != MBB.end() && NextIt->isDebugInstr())
    ++NextIt;
  if (NextIt == MBB.end())
    return false;

  Register Reg = MI.getOperand(0).getReg();
  if (!compareUsesReg(*NextIt, Reg))
    return false;
  if (!isRegAlreadyExtendedBefore(MI.getIterator(), Reg, Signed, Width, TRI))
    return false;

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
  const AVR32Subtarget &STI = MF.getSubtarget<AVR32Subtarget>();
  const TargetInstrInfo &TII = *STI.getInstrInfo();
  const TargetRegisterInfo &TRI = *STI.getRegisterInfo();
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
        case AVR32::MOVEQriCG:
        case AVR32::MOVLTriCG:
          LocalChanged |= foldDoubleCompareLibcall(MI, TII);
          break;
        case AVR32::CPri21:
          LocalChanged |= foldNarrowCompare(MI, TII, TRI) ||
                          foldSignedImmediate(MI, AVR32::CPri6, 6, false,
                                              TII);
          break;
        case AVR32::CPrr:
          LocalChanged |= foldNarrowCompare(MI, TII, TRI) ||
                          foldCompareImmediate(MI, TII);
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
          LocalChanged |= foldNarrowCompare(MI, TII, TRI) ||
                          foldSignedImmediate(MI, AVR32::CP_Wri6, 6, false,
                                              TII);
          break;
        case AVR32::CP_Wrr:
          LocalChanged |= foldNarrowCompare(MI, TII, TRI) ||
                          foldCompareImmediate(MI, TII);
          break;
        case AVR32::CPri6:
        case AVR32::CP_Wri6:
          LocalChanged |= foldNarrowCompare(MI, TII, TRI);
          break;
        case AVR32::CASTS_Bcg:
        case AVR32::CASTS_Hcg:
        case AVR32::CASTU_Bcg:
        case AVR32::CASTU_Hcg:
          LocalChanged |= foldRedundantCastBeforeCompare(MI, TRI);
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
        case AVR32::LD_W_IndexShift:
          LocalChanged |= foldShiftedMaskIndexLoad(MI, TII, TRI);
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
