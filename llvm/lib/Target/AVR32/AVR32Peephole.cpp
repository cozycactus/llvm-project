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
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/LivePhysRegs.h"
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
STATISTIC(NumPredicatedStores, "Number of predicated stores selected");
STATISTIC(NumDoublewordMemOps, "Number of doubleword memory ops selected");

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
  bool foldReverseSub(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldSignedImmediate(MachineInstr &MI, unsigned CompactOpc,
                           unsigned Bits, bool HasDef,
                           const TargetInstrInfo &TII);
  bool foldLoadDisp(MachineInstr &MI, unsigned CompactOpc, unsigned MaxDisp,
                    unsigned Align, const TargetInstrInfo &TII);
  bool foldStoreDisp(MachineInstr &MI, unsigned CompactOpc, unsigned MaxDisp,
                     unsigned Align, const TargetInstrInfo &TII);
  bool foldSPLoadDisp(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldSPStoreDisp(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldDoublewordLoad(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldDoublewordStore(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldMovhOr(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldBitImmediate(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldCompareImmediate(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldDoubleCompareLibcall(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldShiftedMaskIndexLoad(MachineInstr &MI, const TargetInstrInfo &TII,
                                const TargetRegisterInfo &TRI);
  bool foldRotate16(MachineInstr &MI, const TargetInstrInfo &TII);
  bool foldNarrowCompare(MachineInstr &MI, const TargetInstrInfo &TII,
                         const TargetRegisterInfo &TRI);
  bool foldRedundantCastBeforeCompare(MachineInstr &MI,
                                      const TargetRegisterInfo &TRI);
  bool repairDivRemainderClobbers(MachineFunction &MF,
                                  const TargetInstrInfo &TII,
                                  const TargetRegisterInfo &TRI);
  bool repairCompareBranchFlagClobbers(MachineFunction &MF,
                                       const TargetRegisterInfo &TRI);
  bool foldNearBranches(MachineFunction &MF, const TargetInstrInfo &TII);
  bool foldPredicatedStores(MachineFunction &MF, const TargetInstrInfo &TII);
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

static Register getDoublewordHighReg(Register Low) {
  switch (Low) {
  case AVR32::R0:
    return AVR32::R1;
  case AVR32::R2:
    return AVR32::R3;
  case AVR32::R4:
    return AVR32::R5;
  case AVR32::R6:
    return AVR32::R7;
  case AVR32::R8:
    return AVR32::R9;
  case AVR32::R10:
    return AVR32::R11;
  default:
    return Register();
  }
}

static bool hasOrderedMemOperand(const MachineInstr &MI) {
  return llvm::any_of(MI.memoperands(), [](const MachineMemOperand *MMO) {
    return MMO->isVolatile() || MMO->isAtomic();
  });
}

static bool isDoublewordFoldableLoadOpcode(unsigned Opcode) {
  return Opcode == AVR32::LD_W_Disp16 || Opcode == AVR32::LD_W_Disp5 ||
         Opcode == AVR32::LDDSP;
}

static bool isDoublewordFoldableStoreOpcode(unsigned Opcode) {
  return Opcode == AVR32::ST_W_Disp16 || Opcode == AVR32::ST_W_Disp4 ||
         Opcode == AVR32::STDSP;
}

static bool hasSameBaseAndDisp(const MachineInstr &LowAddrMI,
                               const MachineInstr &HighAddrMI, bool IsStore) {
  unsigned BaseIdx = IsStore ? 0 : 1;
  unsigned DispIdx = IsStore ? 1 : 2;
  if (!isRegOperand(LowAddrMI.getOperand(BaseIdx)) ||
      !isRegOperand(HighAddrMI.getOperand(BaseIdx)) ||
      LowAddrMI.getOperand(BaseIdx).getReg() !=
          HighAddrMI.getOperand(BaseIdx).getReg() ||
      !LowAddrMI.getOperand(DispIdx).isImm() ||
      !HighAddrMI.getOperand(DispIdx).isImm())
    return false;

  int64_t LowAddrDisp = LowAddrMI.getOperand(DispIdx).getImm();
  int64_t HighAddrDisp = HighAddrMI.getOperand(DispIdx).getImm();
  return isInt<16>(LowAddrDisp) && HighAddrDisp == LowAddrDisp + 4;
}

static StringRef getDirectCallTargetName(const MachineInstr &MI) {
  if ((MI.getOpcode() != AVR32::RCALLeCG &&
       MI.getOpcode() != AVR32::CALLp) ||
      MI.getNumOperands() == 0)
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

static bool hasLiveSRegDef(const MachineInstr &MI) {
  for (const MachineOperand &MO : MI.operands())
    if (MO.isReg() && MO.isDef() && MO.getReg() == AVR32::SREG &&
        !MO.isDead())
      return true;
  return false;
}

static bool isQuotientOnlyDivOpcode(unsigned Opcode) {
  return Opcode == AVR32::DIVSrrrQ || Opcode == AVR32::DIVUrrrQ;
}

static Register getDivRemainderReg(Register QuotReg) {
  switch (QuotReg.id()) {
  case AVR32::R0:
    return AVR32::R1;
  case AVR32::R2:
    return AVR32::R3;
  case AVR32::R4:
    return AVR32::R5;
  case AVR32::R6:
    return AVR32::R7;
  case AVR32::R8:
    return AVR32::R9;
  case AVR32::R10:
    return AVR32::R11;
  default:
    return Register();
  }
}

static std::optional<Register>
findAvailableDivQuotReg(const LivePhysRegs &Live,
                        const MachineRegisterInfo &MRI) {
  static const MCPhysReg Candidates[] = {AVR32::R8, AVR32::R10, AVR32::R4,
                                         AVR32::R6, AVR32::R0, AVR32::R2};
  for (MCPhysReg Low : Candidates) {
    Register High = getDivRemainderReg(Low);
    if (High && Live.available(MRI, Low) && Live.available(MRI, High))
      return Low;
  }
  return std::nullopt;
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
  return isMovLowImmOpcode(MI.getOpcode()) &&
         MI.getNumExplicitOperands() == 2 && isRegOperand(MI.getOperand(0)) &&
         isLowMaterializationOperand(MI.getOperand(1));
}

static bool isMovhMaterialization(const MachineInstr &MI) {
  return MI.getOpcode() == AVR32::MOVHri &&
         MI.getNumExplicitOperands() == 2 &&
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
  case AVR32::ORLcg:
    return getSingleSetBit(Value & 0xffff);
  case AVR32::ORHri:
  case AVR32::ORHcg:
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
  case AVR32::ANDLcg:
    return getSingleSetBit((~Value) & 0xffff);
  case AVR32::ANDHri:
  case AVR32::ANDHcg:
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

static bool isRegAlreadyExtendedBefore(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator Pos,
                                       Register Reg, bool Signed,
                                       unsigned Width,
                                       const TargetRegisterInfo &TRI,
                                       unsigned Depth);

static bool isRegAlreadyExtendedBefore(MachineBasicBlock::iterator Pos,
                                       Register Reg, bool Signed,
                                       unsigned Width,
                                       const TargetRegisterInfo &TRI,
                                       unsigned Depth = 8) {
  return isRegAlreadyExtendedBefore(*Pos->getParent(), Pos, Reg, Signed, Width,
                                    TRI, Depth);
}

static bool isRegAlreadyExtendedBefore(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator Pos,
                                       Register Reg, bool Signed,
                                       unsigned Width,
                                       const TargetRegisterInfo &TRI,
                                       unsigned Depth) {
  if (!Depth)
    return false;

  for (MachineBasicBlock::iterator I = Pos; I != MBB.begin();) {
    --I;
    MachineInstr &DefMI = *I;
    if (DefMI.isDebugInstr())
      continue;
    if (!DefMI.modifiesRegister(Reg, &TRI))
      continue;
    if (!explicitlyDefinesReg(DefMI, Reg))
      return false;

    if (DefMI.getOpcode() == AVR32::MOVrr &&
        DefMI.getNumExplicitOperands() == 2 &&
        isRegOperand(DefMI.getOperand(0)) && isRegOperand(DefMI.getOperand(1)) &&
        DefMI.getOperand(0).getReg() == Reg)
      return isRegAlreadyExtendedBefore(I, DefMI.getOperand(1).getReg(),
                                        Signed, Width, TRI, Depth - 1);

    if (isMovLowImmOpcode(DefMI.getOpcode()) &&
        DefMI.getNumExplicitOperands() == 2 &&
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

  if (MBB.pred_size() != 1)
    return false;

  MachineBasicBlock *Pred = *MBB.pred_begin();
  if (Pred == &MBB)
    return false;

  return isRegAlreadyExtendedBefore(*Pred, Pred->end(), Reg, Signed, Width, TRI,
                                    Depth - 1);
}

static unsigned getInstSize(const MachineInstr &MI) {
  if (MI.isDebugInstr())
    return 0;
  return MI.getDesc().getSize();
}

enum class AVR32Cond {
  EQ,
  NE,
  CC,
  CS,
  GE,
  LT,
  LS,
  GT,
  LE,
  HI,
};

static std::optional<AVR32Cond> getStoreConditionForSkippedBranch(
    unsigned Opcode) {
  switch (Opcode) {
  case AVR32::BREQbb:
  case AVR32::BREQcbb:
    return AVR32Cond::NE;
  case AVR32::BRNEbb:
  case AVR32::BRNEcbb:
    return AVR32Cond::EQ;
  case AVR32::BRCCbb:
  case AVR32::BRCCcbb:
    return AVR32Cond::CS;
  case AVR32::BRCSbb:
  case AVR32::BRCScbb:
    return AVR32Cond::CC;
  case AVR32::BRGEbb:
  case AVR32::BRGEcbb:
    return AVR32Cond::LT;
  case AVR32::BRLTbb:
  case AVR32::BRLTcbb:
    return AVR32Cond::GE;
  case AVR32::BRLSbb:
    return AVR32Cond::HI;
  case AVR32::BRGTbb:
    return AVR32Cond::LE;
  case AVR32::BRLEbb:
    return AVR32Cond::GT;
  case AVR32::BRHIbb:
    return AVR32Cond::LS;
  default:
    return std::nullopt;
  }
}

static bool fitsPredicatedStoreDisp(unsigned StoreOpcode, int64_t Disp) {
  switch (StoreOpcode) {
  case AVR32::ST_B_Disp3:
  case AVR32::ST_B_Disp16:
    return isCompactDisp(Disp, 511, 1);
  case AVR32::ST_H_Disp3:
  case AVR32::ST_H_Disp16:
    return isCompactDisp(Disp, 1022, 2);
  case AVR32::ST_W_Disp4:
  case AVR32::ST_W_Disp16:
    return isCompactDisp(Disp, 2044, 4);
  default:
    return false;
  }
}

static bool isConditionalBranchOpcode(unsigned Opcode) {
  return getStoreConditionForSkippedBranch(Opcode).has_value();
}

static bool isMovableAcrossCompare(const MachineInstr &MI,
                                   const TargetRegisterInfo &TRI) {
  if (MI.isDebugInstr())
    return true;
  if (MI.isTerminator() || MI.isBranch() || MI.isCall() || MI.isReturn() ||
      MI.isInlineAsm())
    return false;
  if (MI.mayLoad() || MI.mayStore() || MI.hasUnmodeledSideEffects())
    return false;
  return !MI.readsRegister(AVR32::SREG, &TRI);
}

static bool definesAnyCompareInput(const MachineInstr &MI,
                                   const MachineInstr &Cmp,
                                   const TargetRegisterInfo &TRI) {
  for (const MachineOperand &Def : MI.operands()) {
    if (!Def.isReg() || !Def.isDef() || !Def.getReg().isValid())
      continue;
    for (const MachineOperand &Use : Cmp.operands()) {
      if (!Use.isReg() || !Use.isUse() || !Use.getReg().isValid())
        continue;
      if (TRI.regsOverlap(Def.getReg(), Use.getReg()))
        return true;
    }
  }
  return false;
}

static std::optional<unsigned> getPredicatedStoreOpcode(unsigned StoreOpcode,
                                                        AVR32Cond Cond) {
  switch (StoreOpcode) {
  case AVR32::ST_B_Disp3:
  case AVR32::ST_B_Disp16:
    switch (Cond) {
    case AVR32Cond::EQ:
      return AVR32::ST_B_EQ_Disp9;
    case AVR32Cond::NE:
      return AVR32::ST_B_NE_Disp9;
    case AVR32Cond::CC:
      return AVR32::ST_B_CC_Disp9;
    case AVR32Cond::CS:
      return AVR32::ST_B_CS_Disp9;
    case AVR32Cond::GE:
      return AVR32::ST_B_GE_Disp9;
    case AVR32Cond::LT:
      return AVR32::ST_B_LT_Disp9;
    case AVR32Cond::LS:
      return AVR32::ST_B_LS_Disp9;
    case AVR32Cond::GT:
      return AVR32::ST_B_GT_Disp9;
    case AVR32Cond::LE:
      return AVR32::ST_B_LE_Disp9;
    case AVR32Cond::HI:
      return AVR32::ST_B_HI_Disp9;
    }
    llvm_unreachable("unknown AVR32 condition");
  case AVR32::ST_H_Disp3:
  case AVR32::ST_H_Disp16:
    switch (Cond) {
    case AVR32Cond::EQ:
      return AVR32::ST_H_EQ_Disp9;
    case AVR32Cond::NE:
      return AVR32::ST_H_NE_Disp9;
    case AVR32Cond::CC:
      return AVR32::ST_H_CC_Disp9;
    case AVR32Cond::CS:
      return AVR32::ST_H_CS_Disp9;
    case AVR32Cond::GE:
      return AVR32::ST_H_GE_Disp9;
    case AVR32Cond::LT:
      return AVR32::ST_H_LT_Disp9;
    case AVR32Cond::LS:
      return AVR32::ST_H_LS_Disp9;
    case AVR32Cond::GT:
      return AVR32::ST_H_GT_Disp9;
    case AVR32Cond::LE:
      return AVR32::ST_H_LE_Disp9;
    case AVR32Cond::HI:
      return AVR32::ST_H_HI_Disp9;
    }
    llvm_unreachable("unknown AVR32 condition");
  case AVR32::ST_W_Disp4:
  case AVR32::ST_W_Disp16:
    switch (Cond) {
    case AVR32Cond::EQ:
      return AVR32::ST_W_EQ_Disp9;
    case AVR32Cond::NE:
      return AVR32::ST_W_NE_Disp9;
    case AVR32Cond::CC:
      return AVR32::ST_W_CC_Disp9;
    case AVR32Cond::CS:
      return AVR32::ST_W_CS_Disp9;
    case AVR32Cond::GE:
      return AVR32::ST_W_GE_Disp9;
    case AVR32Cond::LT:
      return AVR32::ST_W_LT_Disp9;
    case AVR32Cond::LS:
      return AVR32::ST_W_LS_Disp9;
    case AVR32Cond::GT:
      return AVR32::ST_W_GT_Disp9;
    case AVR32Cond::LE:
      return AVR32::ST_W_LE_Disp9;
    case AVR32Cond::HI:
      return AVR32::ST_W_HI_Disp9;
    }
    llvm_unreachable("unknown AVR32 condition");
  default:
    return std::nullopt;
  }
}

bool AVR32Peephole::foldTwoAddressALU(MachineInstr &MI, unsigned CompactOpc,
                                      bool Commutable,
                                      const TargetInstrInfo &TII) {
  if (MI.getNumExplicitOperands() != 3)
    return false;

  const MachineOperand &Dst = MI.getOperand(0);
  const MachineOperand &LHS = MI.getOperand(1);
  const MachineOperand &RHS = MI.getOperand(2);
  if (!isRegOperand(Dst) || !isRegOperand(LHS) || !isRegOperand(RHS))
    return false;

  Register DstReg = Dst.getReg();
  unsigned SrcIdx = 0;
  unsigned OldIdx = 0;
  if (DstReg == LHS.getReg()) {
    OldIdx = 1;
    SrcIdx = 2;
  } else if (Commutable && DstReg == RHS.getReg()) {
    OldIdx = 2;
    SrcIdx = 1;
  } else {
    return false;
  }

  const MachineOperand &Old = MI.getOperand(OldIdx);
  const MachineOperand &Src = MI.getOperand(SrcIdx);
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineInstrBuilder MIB =
      BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(CompactOpc), DstReg)
          .addReg(DstReg, getKillRegState(Old.isKill()))
          .addReg(Src.getReg(), getKillRegState(Src.isKill()));
  MIB.setMIFlags(MI.getFlags());
  MIB->copyImplicitOps(MF, MI);
  MI.eraseFromParent();
  ++NumCompactForms;
  return true;
}

bool AVR32Peephole::foldReverseSub(MachineInstr &MI,
                                   const TargetInstrInfo &TII) {
  if (MI.getNumExplicitOperands() != 3)
    return false;

  const MachineOperand &Dst = MI.getOperand(0);
  const MachineOperand &LHS = MI.getOperand(1);
  const MachineOperand &RHS = MI.getOperand(2);
  if (!isRegOperand(Dst) || !isRegOperand(LHS) || !isRegOperand(RHS))
    return false;

  Register DstReg = Dst.getReg();
  if (DstReg != RHS.getReg())
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  MachineInstrBuilder MIB =
      BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVR32::RSUBrrCG), DstReg)
          .addReg(DstReg, getKillRegState(RHS.isKill()))
          .addReg(LHS.getReg(), getKillRegState(LHS.isKill()));
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
    if (MI.getNumExplicitOperands() != 3 ||
        !isRegOperand(MI.getOperand(0)) ||
        !isRegOperand(MI.getOperand(1)) || !isRegOperand(MI.getOperand(2)))
      return false;

    Register Src0 = MI.getOperand(1).getReg();
    Register Src1 = MI.getOperand(2).getReg();
    if (!((Src0 == HighReg && Src1 == LowReg) ||
          (Src0 == LowReg && Src1 == HighReg)))
      return false;
    DstReg = MI.getOperand(0).getReg();
  } else if (MI.getOpcode() == AVR32::ORrr) {
    if (MI.getNumExplicitOperands() != 2 ||
        !isRegOperand(MI.getOperand(0)) ||
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
      Opcode == AVR32::ANDLri || Opcode == AVR32::ANDHri ||
      Opcode == AVR32::ORLcg || Opcode == AVR32::ORHcg ||
      Opcode == AVR32::ANDLcg || Opcode == AVR32::ANDHcg) {
    bool IsCodeGenOnly = MI.getNumExplicitOperands() == 3;
    if ((!IsCodeGenOnly && MI.getNumExplicitOperands() != 2) ||
        !isRegOperand(MI.getOperand(0)))
      return false;

    if (IsCodeGenOnly &&
        (!isRegOperand(MI.getOperand(1)) ||
         MI.getOperand(0).getReg() != MI.getOperand(1).getReg()))
      return false;

    const MachineOperand &ImmOp = MI.getOperand(IsCodeGenOnly ? 2 : 1);
    if (!ImmOp.isImm())
      return false;

    Bit = getBitSetIndex(Opcode, ImmOp.getImm());
    CompactOpc = AVR32::SBRri;
    if (!Bit) {
      Bit = getBitClearIndex(Opcode, ImmOp.getImm());
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
  if (MI.getNumExplicitOperands() != 2 ||
      !isRegOperand(MI.getOperand(0)) ||
      !isRegOperand(MI.getOperand(1)) || !MI.getOperand(1).isKill())
    return false;
  if (MI.getIterator() == MI.getParent()->begin())
    return false;

  MachineBasicBlock::iterator ImmIt = std::prev(MI.getIterator());
  MachineInstr &ImmMI = *ImmIt;
  if (!isMovLowImmOpcode(ImmMI.getOpcode()) ||
      ImmMI.getNumExplicitOperands() != 2 ||
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
  if (!ImmOpc || MI.getNumExplicitOperands() != 2 ||
      !isRegOperand(MI.getOperand(0)) ||
      !isRegOperand(MI.getOperand(1)) || !MI.getOperand(1).isKill())
    return false;
  if (MI.getIterator() == MI.getParent()->begin())
    return false;

  MachineBasicBlock::iterator ImmIt = std::prev(MI.getIterator());
  MachineInstr &ImmMI = *ImmIt;
  if (!isMovLowImmOpcode(ImmMI.getOpcode()) ||
      ImmMI.getNumExplicitOperands() != 2 ||
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
  if (MI.getOpcode() == AVR32::SREQr || MI.getOpcode() == AVR32::SRLTr) {
    if (MI.getNumOperands() < 1 || !isRegOperand(MI.getOperand(0)))
      return false;

    MachineBasicBlock &MBB = *MI.getParent();
    MachineBasicBlock::iterator Cmp = prevNonDebug(MI.getIterator());
    if (Cmp == MBB.end())
      return false;

    auto RetargetAndEraseCmp = [&](MachineBasicBlock::iterator Call,
                                   StringRef From, const char *To,
                                   bool SwapArgs) {
      if (Call == MBB.end() || !retargetDirectCall(*Call, From, To))
        return false;
      if (SwapArgs)
        insertF64ArgSwapBefore(*Call, TII);
      Cmp->eraseFromParent();
      MI.setDesc(TII.get(AVR32::SRNEr));
      ++NumCompactForms;
      return true;
    };

    if (isImmediateCompareOpcode(Cmp->getOpcode()) &&
        Cmp->getNumOperands() >= 2 && isRegOperand(Cmp->getOperand(0)) &&
        Cmp->getOperand(0).getReg() == AVR32::R12 &&
        Cmp->getOperand(1).isImm()) {
      int64_t Imm = Cmp->getOperand(1).getImm();
      MachineBasicBlock::iterator Call = prevNonDebug(Cmp);
      if (MI.getOpcode() == AVR32::SREQr && Imm == 0)
        return RetargetAndEraseCmp(Call, "__eqdf2", "__avr32_f64_cmp_eq",
                                   /*SwapArgs=*/false);
      if (MI.getOpcode() == AVR32::SRLTr && Imm == 0)
        return RetargetAndEraseCmp(Call, "__ltdf2", "__avr32_f64_cmp_lt",
                                   /*SwapArgs=*/false);
      if (MI.getOpcode() == AVR32::SRLTr && Imm == 1)
        return RetargetAndEraseCmp(Call, "__ledf2", "__avr32_f64_cmp_ge",
                                   /*SwapArgs=*/true);
      return false;
    }

    if (MI.getOpcode() != AVR32::SRLTr || Cmp->getOpcode() != AVR32::CPrr ||
        Cmp->getNumOperands() < 2 || !isRegOperand(Cmp->getOperand(0)) ||
        !isRegOperand(Cmp->getOperand(1)) ||
        Cmp->getOperand(1).getReg() != AVR32::R12)
      return false;

    Register TestReg = Cmp->getOperand(0).getReg();
    MachineBasicBlock::iterator Imm = prevNonDebug(Cmp);
    if (Imm == MBB.end())
      return false;

    MachineBasicBlock::iterator Call = prevNonDebug(Imm);
    if (isMoveImmToReg(*Imm, TestReg, 0)) {
      if (Call == MBB.end() ||
          !retargetDirectCall(*Call, "__gtdf2", "__avr32_f64_cmp_lt"))
        return false;
      insertF64ArgSwapBefore(*Call, TII);
    } else if (isMoveImmToReg(*Imm, TestReg, -1)) {
      if (Call == MBB.end() ||
          !retargetDirectCall(*Call, "__gedf2", "__avr32_f64_cmp_ge"))
        return false;
    } else {
      return false;
    }

    Imm->eraseFromParent();
    Cmp->eraseFromParent();
    MI.setDesc(TII.get(AVR32::SRNEr));
    ++NumCompactForms;
    return true;
  }

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
    if (I->getOpcode() == AVR32::ANDrr &&
        I->getNumExplicitOperands() == 2 &&
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
      MaskIt->getNumExplicitOperands() != 2 ||
      !isRegOperand(MaskIt->getOperand(0)) ||
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
      ShiftIt->getNumExplicitOperands() != 2 ||
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

static bool isRegRegImmShift16(const MachineInstr &MI, unsigned Opcode,
                               Register Reg) {
  return MI.getOpcode() == Opcode && MI.getNumOperands() >= 3 &&
         isRegOperand(MI.getOperand(0)) && isRegOperand(MI.getOperand(1)) &&
         MI.getOperand(0).getReg() == Reg &&
         MI.getOperand(1).getReg() == Reg && MI.getOperand(2).isImm() &&
         MI.getOperand(2).getImm() == 16;
}

bool AVR32Peephole::foldRotate16(MachineInstr &MI,
                                 const TargetInstrInfo &TII) {
  if (MI.getNumOperands() < 3 || !isRegOperand(MI.getOperand(0)) ||
      !isRegOperand(MI.getOperand(1)) || !isRegOperand(MI.getOperand(2)) ||
      hasLiveSRegDef(MI))
    return false;

  Register DstReg = MI.getOperand(0).getReg();
  Register LHSReg = MI.getOperand(1).getReg();
  Register RHSReg = MI.getOperand(2).getReg();
  Register TmpReg;
  unsigned TmpOpIdx;
  if (LHSReg == DstReg && RHSReg != DstReg) {
    TmpReg = RHSReg;
    TmpOpIdx = 2;
  } else if (MI.getOpcode() == AVR32::ORALrrr && RHSReg == DstReg &&
             LHSReg != DstReg) {
    TmpReg = LHSReg;
    TmpOpIdx = 1;
  } else {
    return false;
  }

  if (!MI.getOperand(TmpOpIdx).isKill())
    return false;

  MachineBasicBlock::iterator SecondShiftIt = prevNonDebug(MI.getIterator());
  if (SecondShiftIt == MI.getParent()->end())
    return false;
  MachineBasicBlock::iterator FirstShiftIt = prevNonDebug(SecondShiftIt);
  if (FirstShiftIt == MI.getParent()->end())
    return false;

  bool FirstIsLsrTmp =
      isRegRegImmShift16(*FirstShiftIt, AVR32::LSRricg, TmpReg);
  bool FirstIsLslTmp =
      isRegRegImmShift16(*FirstShiftIt, AVR32::LSLricg, TmpReg);
  bool FirstIsLslDst =
      isRegRegImmShift16(*FirstShiftIt, AVR32::LSLricg, DstReg);
  bool FirstIsLsrDst =
      isRegRegImmShift16(*FirstShiftIt, AVR32::LSRricg, DstReg);
  bool SecondIsLsrTmp =
      isRegRegImmShift16(*SecondShiftIt, AVR32::LSRricg, TmpReg);
  bool SecondIsLslTmp =
      isRegRegImmShift16(*SecondShiftIt, AVR32::LSLricg, TmpReg);
  bool SecondIsLslDst =
      isRegRegImmShift16(*SecondShiftIt, AVR32::LSLricg, DstReg);
  bool SecondIsLsrDst =
      isRegRegImmShift16(*SecondShiftIt, AVR32::LSRricg, DstReg);
  if (!((FirstIsLsrTmp && SecondIsLslDst) ||
        (FirstIsLslDst && SecondIsLsrTmp) ||
        (FirstIsLslTmp && SecondIsLsrDst) ||
        (FirstIsLsrDst && SecondIsLslTmp)))
    return false;

  MachineBasicBlock::iterator CopyIt = prevNonDebug(FirstShiftIt);
  if (CopyIt == MI.getParent()->end() ||
      CopyIt->getOpcode() != AVR32::MOVrr ||
      CopyIt->getNumExplicitOperands() < 2 ||
      !isRegOperand(CopyIt->getOperand(0)) ||
      !isRegOperand(CopyIt->getOperand(1)) ||
      CopyIt->getOperand(0).getReg() != TmpReg ||
      CopyIt->getOperand(1).getReg() != DstReg)
    return false;

  MachineBasicBlock &MBB = *MI.getParent();
  BuildMI(MBB, CopyIt, CopyIt->getDebugLoc(), TII.get(AVR32::SWAP_Hcg),
          DstReg)
      .addReg(DstReg, getKillRegState(CopyIt->getOperand(1).isKill()))
      .setMIFlags(MI.getFlags());

  CopyIt->eraseFromParent();
  FirstShiftIt->eraseFromParent();
  SecondShiftIt->eraseFromParent();
  MI.eraseFromParent();
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
  if (!isCompareOpcode(MI.getOpcode()) || MI.getNumExplicitOperands() != 2 ||
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
  if (!NarrowOpc || CastIt->getNumExplicitOperands() != 2 ||
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
  if (CopyIt->getOpcode() != AVR32::MOVrr ||
      CopyIt->getNumExplicitOperands() != 2 ||
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

  if (MI.getNumExplicitOperands() != 2 || !isRegOperand(MI.getOperand(0)) ||
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

      if (MI.getOpcode() != AVR32::BRALbb)
        continue;
      CompactOpc = AVR32::RJMPbb;
      MinDisp = -1000;
      MaxDisp = 998;

      if (MI.getNumExplicitOperands() != 1 || !MI.getOperand(0).isMBB())
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

bool AVR32Peephole::foldPredicatedStores(MachineFunction &MF,
                                         const TargetInstrInfo &TII) {
  for (MachineBasicBlock &MBB : MF) {
    if (MBB.empty())
      continue;

    MachineInstr *Branch = nullptr;
    for (MachineInstr &MI : llvm::reverse(MBB)) {
      if (MI.isDebugInstr())
        continue;
      Branch = &MI;
      break;
    }
    if (!Branch || Branch->getNumExplicitOperands() != 1 ||
        !Branch->getOperand(0).isMBB())
      continue;

    std::optional<AVR32Cond> StoreCond =
        getStoreConditionForSkippedBranch(Branch->getOpcode());
    if (!StoreCond)
      continue;

    MachineBasicBlock::iterator Prev = prevNonDebug(Branch->getIterator());
    if (Prev == MBB.end() || !isCompareOpcode(Prev->getOpcode()))
      continue;

    MachineBasicBlock *SkipBB = Branch->getOperand(0).getMBB();
    MachineFunction::iterator StoreIt = std::next(MBB.getIterator());
    if (StoreIt == MF.end())
      continue;
    MachineBasicBlock *StoreBB = &*StoreIt;
    if (std::next(StoreIt) == MF.end() || &*std::next(StoreIt) != SkipBB)
      continue;
    if (StoreBB->succ_size() != 1 || *StoreBB->succ_begin() != SkipBB)
      continue;
    if (!MBB.isSuccessor(StoreBB) || !MBB.isSuccessor(SkipBB))
      continue;
    if (!SkipBB->empty() && SkipBB->begin()->isPHI())
      continue;

    MachineInstr *Store = nullptr;
    bool HasOtherNonDebug = false;
    for (MachineInstr &MI : *StoreBB) {
      if (MI.isDebugInstr())
        continue;
      if (!Store)
        Store = &MI;
      else
        HasOtherNonDebug = true;
    }
    if (!Store || HasOtherNonDebug || Store->getNumExplicitOperands() != 3)
      continue;

    const MachineOperand &Base = Store->getOperand(0);
    const MachineOperand &Disp = Store->getOperand(1);
    const MachineOperand &Src = Store->getOperand(2);
    if (!isRegOperand(Base) || !Disp.isImm() || !isRegOperand(Src) ||
        !fitsPredicatedStoreDisp(Store->getOpcode(), Disp.getImm()))
      continue;

    std::optional<unsigned> CondStoreOpc =
        getPredicatedStoreOpcode(Store->getOpcode(), *StoreCond);
    if (!CondStoreOpc)
      continue;
    if (!Store->mayStore() || Store->mayLoad())
      continue;

    MachineInstrBuilder MIB =
        BuildMI(MBB, *Branch, Store->getDebugLoc(), TII.get(*CondStoreOpc))
            .addReg(Base.getReg(), getKillRegState(Base.isKill()))
            .addImm(Disp.getImm())
            .addReg(Src.getReg(), getKillRegState(Src.isKill()));
    MIB->copyImplicitOps(MF, *Store);
    MIB.setMIFlags(Store->getFlags());
    MIB.setMemRefs(Store->memoperands());

    Branch->eraseFromParent();
    MBB.removeSuccessor(StoreBB);
    StoreBB->removeSuccessor(SkipBB);
    StoreBB->eraseFromParent();

    ++NumPredicatedStores;
    return true;
  }

  return false;
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

bool AVR32Peephole::foldDoublewordLoad(MachineInstr &MI,
                                       const TargetInstrInfo &TII) {
  if (!isDoublewordFoldableLoadOpcode(MI.getOpcode()) ||
      MI.getNumOperands() != 3 || hasOrderedMemOperand(MI))
    return false;

  MachineBasicBlock::iterator PrevIt = prevNonDebug(MI.getIterator());
  if (PrevIt == MI.getParent()->end())
    return false;
  MachineInstr &PrevMI = *PrevIt;
  if (!isDoublewordFoldableLoadOpcode(PrevMI.getOpcode()) ||
      PrevMI.getNumOperands() != 3 || hasOrderedMemOperand(PrevMI))
    return false;

  const MachineInstr *LowAddrMI = nullptr;
  const MachineOperand *LowDst = nullptr;
  const MachineOperand *HighDst = nullptr;

  const MachineOperand &PrevDst = PrevMI.getOperand(0);
  const MachineOperand &CurDst = MI.getOperand(0);
  if (!isRegOperand(PrevDst) || !isRegOperand(CurDst))
    return false;

  Register PrevReg = PrevDst.getReg();
  Register CurReg = CurDst.getReg();
  if (getDoublewordHighReg(CurReg) == PrevReg &&
      hasSameBaseAndDisp(PrevMI, MI, false)) {
    LowAddrMI = &PrevMI;
    LowDst = &CurDst;
    HighDst = &PrevDst;
  } else if (getDoublewordHighReg(PrevReg) == CurReg &&
             hasSameBaseAndDisp(MI, PrevMI, false)) {
    LowAddrMI = &MI;
    LowDst = &PrevDst;
    HighDst = &CurDst;
  } else {
    return false;
  }

  Register LowReg = LowDst->getReg();
  Register HighReg = HighDst->getReg();

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  SmallVector<MachineMemOperand *, 2> MemRefs;
  llvm::append_range(MemRefs, PrevMI.memoperands());
  llvm::append_range(MemRefs, MI.memoperands());

  MachineInstrBuilder MIB =
      BuildMI(MBB, PrevMI, PrevMI.getDebugLoc(), TII.get(AVR32::LD_D_Disp16))
          .addReg(LowReg,
                  RegState::Define | getDeadRegState(LowDst->isDead()))
          .addReg(LowAddrMI->getOperand(1).getReg(),
                  getKillRegState(PrevMI.getOperand(1).isKill() ||
                                  MI.getOperand(1).isKill()))
          .addImm(LowAddrMI->getOperand(2).getImm())
          .addReg(HighReg, RegState::Implicit | RegState::Define |
                               getDeadRegState(HighDst->isDead()));
  MIB.setMIFlags(PrevMI.getFlags());
  MIB->copyImplicitOps(MF, PrevMI);
  MIB.setMemRefs(MemRefs);

  PrevMI.eraseFromParent();
  MI.eraseFromParent();
  ++NumDoublewordMemOps;
  return true;
}

bool AVR32Peephole::foldDoublewordStore(MachineInstr &MI,
                                        const TargetInstrInfo &TII) {
  if (!isDoublewordFoldableStoreOpcode(MI.getOpcode()) ||
      MI.getNumOperands() != 3 || hasOrderedMemOperand(MI))
    return false;

  MachineBasicBlock::iterator PrevIt = prevNonDebug(MI.getIterator());
  if (PrevIt == MI.getParent()->end())
    return false;
  MachineInstr &PrevMI = *PrevIt;
  if (!isDoublewordFoldableStoreOpcode(PrevMI.getOpcode()) ||
      PrevMI.getNumOperands() != 3 || hasOrderedMemOperand(PrevMI))
    return false;

  const MachineInstr *LowAddrMI = nullptr;
  const MachineOperand *LowSrc = nullptr;
  const MachineOperand *HighSrc = nullptr;

  const MachineOperand &PrevSrc = PrevMI.getOperand(2);
  const MachineOperand &CurSrc = MI.getOperand(2);
  if (!isRegOperand(PrevSrc) || !isRegOperand(CurSrc))
    return false;

  Register PrevReg = PrevSrc.getReg();
  Register CurReg = CurSrc.getReg();
  if (getDoublewordHighReg(CurReg) == PrevReg &&
      hasSameBaseAndDisp(PrevMI, MI, true)) {
    LowAddrMI = &PrevMI;
    LowSrc = &CurSrc;
    HighSrc = &PrevSrc;
  } else if (getDoublewordHighReg(PrevReg) == CurReg &&
             hasSameBaseAndDisp(MI, PrevMI, true)) {
    LowAddrMI = &MI;
    LowSrc = &PrevSrc;
    HighSrc = &CurSrc;
  } else {
    return false;
  }

  Register LowReg = LowSrc->getReg();
  Register HighReg = HighSrc->getReg();

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  SmallVector<MachineMemOperand *, 2> MemRefs;
  llvm::append_range(MemRefs, PrevMI.memoperands());
  llvm::append_range(MemRefs, MI.memoperands());

  MachineInstrBuilder MIB =
      BuildMI(MBB, PrevMI, PrevMI.getDebugLoc(), TII.get(AVR32::ST_D_Disp16))
          .addReg(LowAddrMI->getOperand(0).getReg(),
                  getKillRegState(PrevMI.getOperand(0).isKill() ||
                                  MI.getOperand(0).isKill()))
          .addImm(LowAddrMI->getOperand(1).getImm())
          .addReg(LowReg, getKillRegState(LowSrc->isKill()))
          .addReg(HighReg,
                  RegState::Implicit | getKillRegState(HighSrc->isKill()));
  MIB.setMIFlags(PrevMI.getFlags());
  MIB->copyImplicitOps(MF, PrevMI);
  MIB.setMemRefs(MemRefs);

  PrevMI.eraseFromParent();
  MI.eraseFromParent();
  ++NumDoublewordMemOps;
  return true;
}

bool AVR32Peephole::repairCompareBranchFlagClobbers(
    MachineFunction &MF, const TargetRegisterInfo &TRI) {
  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    for (MachineBasicBlock::iterator Branch = MBB.begin(), E = MBB.end();
         Branch != E; ++Branch) {
      if (!isConditionalBranchOpcode(Branch->getOpcode()))
        continue;

      MachineBasicBlock::iterator Scan = Branch;
      bool HasFlagClobber = false;
      while (Scan != MBB.begin()) {
        MachineBasicBlock::iterator Prev = std::prev(Scan);
        if (Prev->isDebugInstr()) {
          Scan = Prev;
          continue;
        }

        if (isCompareOpcode(Prev->getOpcode())) {
          if (!HasFlagClobber)
            break;

          MachineBasicBlock::iterator WindowBegin = std::next(Prev);
          bool CanMoveWindow = true;
          for (MachineBasicBlock::iterator It = WindowBegin; It != Branch;
               ++It) {
            if (!isMovableAcrossCompare(*It, TRI) ||
                definesAnyCompareInput(*It, *Prev, TRI)) {
              CanMoveWindow = false;
              break;
            }
          }
          if (!CanMoveWindow)
            break;

          Prev->clearKillInfo();
          for (MachineBasicBlock::iterator It = WindowBegin; It != Branch;
               ++It)
            It->clearKillInfo();
          MBB.splice(Prev, &MBB, WindowBegin, Branch);
          Changed = true;
          break;
        }

        if (!isMovableAcrossCompare(*Prev, TRI))
          break;
        if (Prev->modifiesRegister(AVR32::SREG, &TRI))
          HasFlagClobber = true;
        Scan = Prev;
      }
    }
  }

  return Changed;
}

bool AVR32Peephole::foldSignedImmediate(MachineInstr &MI, unsigned CompactOpc,
                                        unsigned Bits, bool HasDef,
                                        const TargetInstrInfo &TII) {
  if (MI.getNumExplicitOperands() != 2)
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

bool AVR32Peephole::repairDivRemainderClobbers(
    MachineFunction &MF, const TargetInstrInfo &TII,
    const TargetRegisterInfo &TRI) {
  MachineRegisterInfo &MRI = MF.getRegInfo();
  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    LivePhysRegs Live(TRI);
    Live.addLiveOutsNoPristines(MBB);

    for (MachineBasicBlock::iterator I = MBB.end(); I != MBB.begin();) {
      --I;
      MachineInstr &MI = *I;
      MachineInstr *InsertedCopy = nullptr;
      MachineInstr *InsertedReload = nullptr;

      if (isQuotientOnlyDivOpcode(MI.getOpcode()) && MI.getNumOperands() >= 3 &&
          isRegOperand(MI.getOperand(0))) {
        Register QuotReg = MI.getOperand(0).getReg();
        Register RemReg = getDivRemainderReg(QuotReg);
        if (RemReg && Live.contains(RemReg)) {
          std::optional<Register> AltQuotReg =
              findAvailableDivQuotReg(Live, MRI);
          if (AltQuotReg) {
            MI.getOperand(0).setReg(*AltQuotReg);
            MI.clearKillInfo();
            InsertedCopy =
                BuildMI(MBB, std::next(MI.getIterator()), MI.getDebugLoc(),
                        TII.get(AVR32::MOVrr), QuotReg)
                    .addReg(*AltQuotReg);
          } else {
            BuildMI(MBB, MI, MI.getDebugLoc(), TII.get(AVR32::ST_W_PreDec))
                .addReg(AVR32::SP)
                .addReg(RemReg);
            InsertedReload =
                BuildMI(MBB, std::next(MI.getIterator()), MI.getDebugLoc(),
                        TII.get(AVR32::LD_W_PostInc), RemReg)
                    .addReg(AVR32::SP, RegState::Define)
                    .addReg(AVR32::SP);
          }
          Changed = true;
        }
      }

      if (InsertedCopy)
        Live.stepBackward(*InsertedCopy);
      if (InsertedReload)
        Live.stepBackward(*InsertedReload);
      Live.stepBackward(MI);
    }
  }

  return Changed;
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
        case AVR32::ADDCrrr:
        case AVR32::ADDALrrr:
          LocalChanged |= foldTwoAddressALU(MI, AVR32::ADDrrCG, true, TII);
          break;
        case AVR32::ANDALrrr:
          LocalChanged |= foldTwoAddressALU(MI, AVR32::ANDrrCG, true, TII);
          break;
        case AVR32::ANDrr:
          LocalChanged |= foldBitImmediate(MI, TII);
          break;
        case AVR32::ANDLri:
        case AVR32::ANDHri:
        case AVR32::ANDLcg:
        case AVR32::ANDHcg:
          LocalChanged |= foldBitImmediate(MI, TII);
          break;
        case AVR32::EORALrrr:
          LocalChanged |= foldTwoAddressALU(MI, AVR32::EORrrCG, true, TII);
          break;
        case AVR32::MULrrr:
          LocalChanged |= foldTwoAddressALU(MI, AVR32::MULrrCG, true, TII);
          break;
        case AVR32::MOVri21:
          LocalChanged |= foldSignedImmediate(MI, AVR32::MOVri8, 8, true, TII);
          break;
        case AVR32::MOVEQriCG:
        case AVR32::MOVLTriCG:
        case AVR32::SREQr:
        case AVR32::SRLTr:
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
          LocalChanged |= foldRotate16(MI, TII) || foldMovhOr(MI, TII) ||
                          foldTwoAddressALU(MI, AVR32::ORrrCG, true, TII);
          break;
        case AVR32::ORrr:
          LocalChanged |= foldMovhOr(MI, TII) || foldBitImmediate(MI, TII);
          break;
        case AVR32::ORrrCG:
          LocalChanged |= foldRotate16(MI, TII);
          break;
        case AVR32::ORLri:
        case AVR32::ORHri:
        case AVR32::ORLcg:
        case AVR32::ORHcg:
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
          LocalChanged |= foldDoublewordLoad(MI, TII) ||
                          foldSPLoadDisp(MI, TII) ||
                          foldLoadDisp(MI, AVR32::LD_W_Disp5, 124, 4, TII);
          break;
        case AVR32::ST_B_Disp16:
          LocalChanged |= foldStoreDisp(MI, AVR32::ST_B_Disp3, 7, 1, TII);
          break;
        case AVR32::ST_H_Disp16:
          LocalChanged |= foldStoreDisp(MI, AVR32::ST_H_Disp3, 14, 2, TII);
          break;
        case AVR32::ST_W_Disp16:
          LocalChanged |= foldDoublewordStore(MI, TII) ||
                          foldSPStoreDisp(MI, TII) ||
                          foldStoreDisp(MI, AVR32::ST_W_Disp4, 60, 4, TII);
          break;
        case AVR32::SUBALrrr:
          LocalChanged |= foldTwoAddressALU(MI, AVR32::SUBrrCG, false, TII) ||
                          foldReverseSub(MI, TII);
          break;
        case AVR32::SUBCrrr:
          LocalChanged |= foldTwoAddressALU(MI, AVR32::SUBrrCG, false, TII);
          break;
        default:
          break;
        }
      }
    }
    Changed |= LocalChanged;
  } while (LocalChanged);

  if (repairCompareBranchFlagClobbers(MF, TRI))
    Changed = true;

  if (repairDivRemainderClobbers(MF, TII, TRI))
    Changed = true;

  if (STI.hasCondStore()) {
    while (foldPredicatedStores(MF, TII))
      Changed = true;
  }

  while (foldNearBranches(MF, TII))
    Changed = true;

  return Changed;
}

FunctionPass *llvm::createAVR32PeepholePass() { return new AVR32Peephole(); }
