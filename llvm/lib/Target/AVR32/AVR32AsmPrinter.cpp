//===-- AVR32AsmPrinter.cpp - AVR32 LLVM assembly writer ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32.h"
#include "MCTargetDesc/AVR32InstPrinter.h"
#include "MCTargetDesc/AVR32TargetStreamer.h"
#include "TargetInfo/AVR32TargetInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"
#include <limits>

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#define GET_INSTRINFO_ENUM
#include "AVR32GenInstrInfo.inc"
#define GET_REGINFO_ENUM
#include "AVR32GenRegisterInfo.inc"

static bool shouldUseCompactLdaW(const MachineFunction &MF,
                                 const MachineInstr &LoadMI);

namespace {
class AVR32AsmPrinter : public AsmPrinter {
  DenseSet<const MachineInstr *> CompactLdaWs;
  DenseSet<const MachineBasicBlock *> PoolBeforeBlocks;
  bool HasLdaWPoolPlan = false;
  unsigned PendingLdaW = 0;

public:
  AVR32AsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  StringRef getPassName() const override { return "AVR32 Assembly Printer"; }

  void emitFunctionBodyStart() override;
  void emitBasicBlockStart(const MachineBasicBlock &MBB) override;
  void emitInstruction(const MachineInstr *MI) override;
  void emitFunctionBodyEnd() override;
  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       const char *ExtraCode, raw_ostream &O) override;
  bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo,
                             const char *ExtraCode, raw_ostream &O) override;
  const MCExpr *lowerSymbolOperand(const MachineOperand &MO);
  AVR32TargetStreamer &getAVR32TargetStreamer() const;
  void buildLdaWPoolPlan(const MachineFunction &MF);
  void emitCallPseudo(const MachineInstr *MI);
  void emitLdaWPseudo(const MachineInstr *MI);
  void emitPendingConstantPool();
  bool shouldOmitFallthroughBranch(const MachineInstr *MI) const;

  static char ID;
};
} // namespace

AVR32TargetStreamer &AVR32AsmPrinter::getAVR32TargetStreamer() const {
  MCTargetStreamer *TS = OutStreamer->getTargetStreamer();
  assert(TS && "missing AVR32 target streamer");
  return static_cast<AVR32TargetStreamer &>(*TS);
}

const MCExpr *AVR32AsmPrinter::lowerSymbolOperand(const MachineOperand &MO) {
  const MCSymbol *Symbol;
  int64_t Offset = 0;
  switch (MO.getType()) {
  case MachineOperand::MO_GlobalAddress:
    Symbol = getSymbol(MO.getGlobal());
    Offset = MO.getOffset();
    break;
  case MachineOperand::MO_ExternalSymbol:
    Symbol = GetExternalSymbolSymbol(MO.getSymbolName());
    Offset = MO.getOffset();
    break;
  case MachineOperand::MO_MachineBasicBlock:
    Symbol = MO.getMBB()->getSymbol();
    break;
  case MachineOperand::MO_BlockAddress:
    Symbol = GetBlockAddressSymbol(MO.getBlockAddress());
    Offset = MO.getOffset();
    break;
  case MachineOperand::MO_JumpTableIndex:
    Symbol = GetJTISymbol(MO.getIndex());
    break;
  default:
    llvm_unreachable("Unexpected symbol operand");
  }

  const MCExpr *Expr = MCSymbolRefExpr::create(Symbol, OutContext);
  if (Offset != 0)
    Expr = MCBinaryExpr::createAdd(
        Expr, MCConstantExpr::create(Offset, OutContext), OutContext);
  switch (MO.getTargetFlags()) {
  case AVR32II::MO_NO_FLAG:
    break;
  case AVR32II::MO_ABS_HI:
    Expr = MCSpecifierExpr::create(Expr, ELF::R_AVR32_HI16, OutContext);
    break;
  case AVR32II::MO_ABS_LO:
    Expr = MCSpecifierExpr::create(Expr, ELF::R_AVR32_LO16, OutContext);
    break;
  default:
    llvm_unreachable("Unexpected AVR32 symbol operand target flag");
  }
  return Expr;
}

bool AVR32AsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                      const char *ExtraCode, raw_ostream &O) {
  if (ExtraCode && ExtraCode[0])
    return true;

  const MachineOperand &MO = MI->getOperand(OpNo);
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    O << AVR32InstPrinter::getRegisterName(MO.getReg());
    return false;
  case MachineOperand::MO_Immediate:
    O << MO.getImm();
    return false;
  case MachineOperand::MO_GlobalAddress:
    O << *getSymbol(MO.getGlobal());
    return false;
  case MachineOperand::MO_ExternalSymbol:
    O << *GetExternalSymbolSymbol(MO.getSymbolName());
    return false;
  case MachineOperand::MO_MachineBasicBlock:
    O << *MO.getMBB()->getSymbol();
    return false;
  case MachineOperand::MO_BlockAddress:
    GetBlockAddressSymbol(MO.getBlockAddress())->print(O, MAI);
    return false;
  case MachineOperand::MO_JumpTableIndex:
    O << *GetJTISymbol(MO.getIndex());
    return false;
  default:
    break;
  }

  return true;
}

bool AVR32AsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                            unsigned OpNo,
                                            const char *ExtraCode,
                                            raw_ostream &O) {
  if (ExtraCode && ExtraCode[0])
    return true;
  if (OpNo + 1 >= MI->getNumOperands())
    return true;

  const MachineOperand &Base = MI->getOperand(OpNo);
  const MachineOperand &Disp = MI->getOperand(OpNo + 1);
  if (!Base.isReg())
    return true;

  O << AVR32InstPrinter::getRegisterName(Base.getReg()) << '[';
  if (Disp.isImm())
    O << Disp.getImm();
  else if (PrintAsmOperand(MI, OpNo + 1, nullptr, O))
    return true;
  O << ']';
  return false;
}

void AVR32AsmPrinter::emitPendingConstantPool() {
  if (!PendingLdaW)
    return;

  getAVR32TargetStreamer().emitCurrentConstantPool();
  PendingLdaW = 0;
}

void AVR32AsmPrinter::emitFunctionBodyStart() {
  buildLdaWPoolPlan(*MF);
}

void AVR32AsmPrinter::emitBasicBlockStart(const MachineBasicBlock &MBB) {
  if (HasLdaWPoolPlan && PoolBeforeBlocks.contains(&MBB))
    emitPendingConstantPool();
  AsmPrinter::emitBasicBlockStart(MBB);
}

void AVR32AsmPrinter::emitFunctionBodyEnd() {
  emitPendingConstantPool();
  CompactLdaWs.clear();
  PoolBeforeBlocks.clear();
  HasLdaWPoolPlan = false;
}

void AVR32AsmPrinter::emitLdaWPseudo(const MachineInstr *MI) {
  const MachineOperand &Dst = MI->getOperand(0);
  const MachineOperand &Value = MI->getOperand(1);
  assert(Dst.isReg() && "lda.w destination must be a register");

  const MCExpr *ValueExpr = nullptr;
  if (Value.isGlobal() || Value.isSymbol() || Value.isMBB() ||
      Value.isBlockAddress() || Value.isJTI())
    ValueExpr = lowerSymbolOperand(Value);
  else if (Value.isImm())
    ValueExpr = MCConstantExpr::create(Value.getImm(), OutContext);
  else
    report_fatal_error("AVR32 lda.w cannot lower this operand yet");

  const MCExpr *CPLoc =
      getAVR32TargetStreamer().addCPENTConstantPoolEntry(ValueExpr, SMLoc());
  ++PendingLdaW;

  MCInst Load;
  bool UseCompact = HasLdaWPoolPlan ? CompactLdaWs.contains(MI)
                                    : shouldUseCompactLdaW(*MF, *MI);
  Load.setOpcode(UseCompact ? AVR32::LDDPCcp : AVR32::LDDPC_PCREL16);
  Load.addOperand(MCOperand::createReg(Dst.getReg()));
  Load.addOperand(MCOperand::createReg(AVR32::PC));
  Load.addOperand(MCOperand::createExpr(CPLoc));
  EmitToStreamer(*OutStreamer, Load);
}

void AVR32AsmPrinter::emitCallPseudo(const MachineInstr *MI) {
  const MachineOperand &Value = MI->getOperand(0);

  const MCExpr *ValueExpr = nullptr;
  if (Value.isGlobal() || Value.isSymbol() || Value.isMBB() ||
      Value.isBlockAddress() || Value.isJTI())
    ValueExpr = lowerSymbolOperand(Value);
  else if (Value.isImm())
    ValueExpr = MCConstantExpr::create(Value.getImm(), OutContext);
  else
    report_fatal_error("AVR32 call cannot lower this operand yet");

  const MCExpr *CPLoc =
      getAVR32TargetStreamer().addCPENTConstantPoolEntry(
          ValueExpr, SMLoc(), /*CanShare=*/false);
  ++PendingLdaW;

  MCInst Call;
  Call.setOpcode(AVR32::MCALLcp);
  Call.addOperand(MCOperand::createReg(AVR32::PC));
  Call.addOperand(MCOperand::createExpr(CPLoc));
  EmitToStreamer(*OutStreamer, Call);
}

static bool isMBBBranchOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVR32::BRALbb:
  case AVR32::RJMPbb:
  case AVR32::BREQbb:
  case AVR32::BRNEbb:
  case AVR32::BRCCbb:
  case AVR32::BRCSbb:
  case AVR32::BRGEbb:
  case AVR32::BRLTbb:
  case AVR32::BREQcbb:
  case AVR32::BRNEcbb:
  case AVR32::BRCCcbb:
  case AVR32::BRCScbb:
  case AVR32::BRGEcbb:
  case AVR32::BRLTcbb:
    return true;
  default:
    return false;
  }
}

bool AVR32AsmPrinter::shouldOmitFallthroughBranch(
    const MachineInstr *MI) const {
  if (!isMBBBranchOpcode(MI->getOpcode()) ||
      MI->getNumExplicitOperands() != 1 || !MI->getOperand(0).isMBB())
    return false;

  const MachineBasicBlock *MBB = MI->getParent();
  const MachineBasicBlock *Target = MI->getOperand(0).getMBB();
  auto NextMBB = MBB->getIterator();
  ++NextMBB;
  if (NextMBB == MBB->getParent()->end() || &*NextMBB != Target)
    return false;

  return !HasLdaWPoolPlan || !PoolBeforeBlocks.contains(Target);
}

void AVR32AsmPrinter::emitInstruction(const MachineInstr *MI) {
  if (MI->getOpcode() == AVR32::CALLp)
    return emitCallPseudo(MI);
  if (MI->getOpcode() == AVR32::LDA_W)
    return emitLdaWPseudo(MI);
  if (shouldOmitFallthroughBranch(MI))
    return;

  MCInst Inst;
  Inst.setOpcode(MI->getOpcode());
  for (const MachineOperand &MO : MI->operands()) {
    if (MO.isReg() && MO.isImplicit())
      continue;
    if (MO.isReg())
      Inst.addOperand(MCOperand::createReg(MO.getReg()));
    else if (MO.isImm())
      Inst.addOperand(MCOperand::createImm(MO.getImm()));
    else if (MO.isGlobal() || MO.isSymbol() || MO.isMBB() ||
             MO.isBlockAddress() || MO.isJTI())
      Inst.addOperand(MCOperand::createExpr(lowerSymbolOperand(MO)));
    else if (MO.isRegMask())
      continue;
    else
      report_fatal_error("AVR32 asm printer cannot lower this operand yet");
  }
  EmitToStreamer(*OutStreamer, Inst);
}

char AVR32AsmPrinter::ID = 0;

static uint64_t align4(uint64_t Value) {
  return (Value + 3) & ~uint64_t(3);
}

static bool isConstantPoolBoundaryOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVR32::BRALbb:
  case AVR32::RJMPbb:
  case AVR32::LD_W_JT:
  case AVR32::POPM_RET:
  case AVR32::POPM_RETVAL:
  case AVR32::RETALr:
  case AVR32::RETD:
  case AVR32::RETE:
  case AVR32::RETJ:
  case AVR32::RETR12:
  case AVR32::RETS:
  case AVR32::RETSS:
    return true;
  default:
    return false;
  }
}

static bool endsWithConstantPoolBoundary(const MachineBasicBlock &MBB) {
  for (auto I = MBB.instr_rbegin(), E = MBB.instr_rend(); I != E; ++I) {
    if (I->isDebugInstr() || I->isMetaInstruction())
      continue;
    return isConstantPoolBoundaryOpcode(I->getOpcode());
  }
  return false;
}

static bool getCompactBranchRange(unsigned Opcode, int64_t &MinDisp,
                                  int64_t &MaxDisp) {
  switch (Opcode) {
  case AVR32::RJMPbb:
    MinDisp = -1000;
    MaxDisp = 998;
    return true;
  case AVR32::BREQcbb:
  case AVR32::BRNEcbb:
  case AVR32::BRCCcbb:
  case AVR32::BRCScbb:
  case AVR32::BRGEcbb:
  case AVR32::BRLTcbb:
    MinDisp = -248;
    MaxDisp = 246;
    return true;
  default:
    return false;
  }
}

struct PlannedPool {
  uint64_t Offset;
  uint64_t Bytes;
};

static constexpr uint64_t MaxFullLddpcPoolDistance = 28 * 1024;

static uint64_t poolShiftBefore(ArrayRef<PlannedPool> Pools, uint64_t Offset) {
  uint64_t Shift = 0;
  for (const PlannedPool &Pool : Pools)
    if (Offset >= Pool.Offset)
      Shift += Pool.Bytes;
  return Shift;
}

static bool compactBranchOutOfRangeWithPools(
    const MachineFunction &MF,
    const DenseMap<const MachineBasicBlock *, uint64_t> &BlockOffsets,
    const DenseMap<const MachineInstr *, uint64_t> &InstrOffsets,
    ArrayRef<PlannedPool> Pools) {
  for (const MachineBasicBlock &MBB : MF) {
    for (const MachineInstr &MI : MBB) {
      int64_t MinDisp = 0;
      int64_t MaxDisp = 0;
      if (!getCompactBranchRange(MI.getOpcode(), MinDisp, MaxDisp) ||
          MI.getNumExplicitOperands() != 1 || !MI.getOperand(0).isMBB())
        continue;

      uint64_t BranchOffset = InstrOffsets.lookup(&MI);
      uint64_t TargetOffset = BlockOffsets.lookup(MI.getOperand(0).getMBB());
      int64_t NewBranchOffset = static_cast<int64_t>(
          BranchOffset + poolShiftBefore(Pools, BranchOffset));
      int64_t NewTargetOffset = static_cast<int64_t>(
          TargetOffset + poolShiftBefore(Pools, TargetOffset));

      int64_t Disp = NewTargetOffset - NewBranchOffset;
      if (Disp < MinDisp || Disp > MaxDisp || (Disp & 1))
        return true;
    }
  }
  return false;
}

struct LiteralPoolUse {
  const MachineInstr *MI;
  uint64_t Offset;
  unsigned LiteralIndex;
  bool IsLdaW;
};

static uint64_t literalPoolEntryDistance(uint64_t PoolOffset,
                                         const LiteralPoolUse &Use,
                                         ArrayRef<PlannedPool> Pools = {}) {
  uint64_t ShiftedPoolOffset =
      PoolOffset + poolShiftBefore(Pools, PoolOffset);
  uint64_t ShiftedUseOffset = Use.Offset + poolShiftBefore(Pools, Use.Offset);
  if (ShiftedPoolOffset < ShiftedUseOffset)
    return std::numeric_limits<uint64_t>::max();

  return align4(ShiftedPoolOffset) - ShiftedUseOffset +
         uint64_t(Use.LiteralIndex + 1) * 4;
}

static bool compactLdaWFits(uint64_t PoolOffset, const LiteralPoolUse &Use,
                            ArrayRef<PlannedPool> Pools = {}) {
  return Use.IsLdaW && literalPoolEntryDistance(PoolOffset, Use, Pools) <= 508;
}

static bool needsRangePool(uint64_t PoolOffset, const LiteralPoolUse &Use,
                           ArrayRef<PlannedPool> Pools = {}) {
  return Use.IsLdaW && literalPoolEntryDistance(PoolOffset, Use, Pools) >
                            MaxFullLddpcPoolDistance;
}

static bool getKnownInstSize(const MachineFunction &MF, const MachineInstr &MI,
                             unsigned &Size) {
  if (MI.isDebugInstr() || MI.isMetaInstruction()) {
    Size = 0;
    return true;
  }

  if (MI.isInlineAsm()) {
    const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
    const MCAsmInfo *MAI = MF.getTarget().getMCAsmInfo();
    if (!TII || !MAI || MI.getNumOperands() == 0 ||
        !MI.getOperand(0).isSymbol())
      return false;
    Size = TII->getInlineAsmLength(MI.getOperand(0).getSymbolName(), *MAI,
                                   &MF.getSubtarget());
    return true;
  }

  Size = MI.getDesc().getSize();
  return Size != 0;
}

void AVR32AsmPrinter::buildLdaWPoolPlan(const MachineFunction &MF) {
  CompactLdaWs.clear();
  PoolBeforeBlocks.clear();
  HasLdaWPoolPlan = false;
  PendingLdaW = 0;

  for (unsigned Iter = 0; Iter != 8; ++Iter) {
    DenseMap<const MachineBasicBlock *, uint64_t> BlockOffsets;
    DenseMap<const MachineInstr *, uint64_t> InstrOffsets;
    uint64_t CodeBytes = 0;

    for (const MachineBasicBlock &MBB : MF) {
      BlockOffsets[&MBB] = CodeBytes;
      for (const MachineInstr &MI : MBB) {
        if (MI.isDebugInstr() || MI.isMetaInstruction())
          continue;

        unsigned Size = 0;
        if (!getKnownInstSize(MF, MI, Size))
          return;
        if (MI.getOpcode() == AVR32::LDA_W && CompactLdaWs.contains(&MI))
          Size = 2;

        InstrOffsets[&MI] = CodeBytes;
        CodeBytes += Size;
      }
    }

    DenseSet<const MachineInstr *> NextCompactLdaWs;
    DenseSet<const MachineBasicBlock *> NextPoolBeforeBlocks;
    SmallVector<PlannedPool, 16> PlannedPools;

    SmallVector<LiteralPoolUse, 32> Pending;
    DenseMap<MachineOperand, unsigned> PendingPoolEntryIndices;
    unsigned PendingPoolEntryCount = 0;
    auto GetOrAddPendingPoolEntry = [&](const MachineOperand &MO) {
      auto [It, Inserted] = PendingPoolEntryIndices.try_emplace(
          MO, PendingPoolEntryCount);
      if (Inserted)
        ++PendingPoolEntryCount;
      return It->second;
    };
    auto AddUniquePendingPoolEntry = [&]() {
      return PendingPoolEntryCount++;
    };

    for (const MachineBasicBlock &MBB : MF) {
      uint64_t BlockEnd = BlockOffsets.lookup(&MBB);
      for (const MachineInstr &MI : MBB) {
        if (MI.isDebugInstr() || MI.isMetaInstruction())
          continue;
        if (MI.getOpcode() == AVR32::LDA_W) {
          unsigned LiteralIndex = GetOrAddPendingPoolEntry(MI.getOperand(1));
          Pending.push_back(
              {&MI, InstrOffsets.lookup(&MI), LiteralIndex, /*IsLdaW=*/true});
        } else if (MI.getOpcode() == AVR32::CALLp) {
          unsigned LiteralIndex = AddUniquePendingPoolEntry();
          Pending.push_back({&MI, InstrOffsets.lookup(&MI), LiteralIndex,
                             /*IsLdaW=*/false});
        }
        unsigned Size = 0;
        if (!getKnownInstSize(MF, MI, Size))
          return;
        if (MI.getOpcode() == AVR32::LDA_W && CompactLdaWs.contains(&MI))
          Size = 2;
        BlockEnd += Size;
      }

      if (Pending.empty() || !endsWithConstantPoolBoundary(MBB))
        continue;

      auto MBBI = MBB.getIterator();
      auto NextMBB = MBBI;
      ++NextMBB;
      if (NextMBB == MF.end())
        continue;

      uint64_t ShiftedBlockEnd =
          BlockEnd + poolShiftBefore(PlannedPools, BlockEnd);
      uint64_t PoolBytes = align4(ShiftedBlockEnd) - ShiftedBlockEnd +
                           uint64_t(PendingPoolEntryCount) * 4;
      SmallVector<PlannedPool, 16> CandidatePools(PlannedPools);
      CandidatePools.push_back({BlockEnd, PoolBytes});
      if (compactBranchOutOfRangeWithPools(MF, BlockOffsets, InstrOffsets,
                                           CandidatePools))
        continue;

      unsigned NewCompactLoads = 0;
      for (const LiteralPoolUse &Use : Pending)
        if (!CompactLdaWs.contains(Use.MI) &&
            compactLdaWFits(BlockEnd, Use, PlannedPools))
          ++NewCompactLoads;

      bool RangePoolNeeded = false;
      for (const LiteralPoolUse &Use : Pending) {
        if (needsRangePool(BlockEnd, Use, PlannedPools)) {
          RangePoolNeeded = true;
          break;
        }
      }

      bool KeepExistingPool = PoolBeforeBlocks.contains(&*NextMBB);
      if (!KeepExistingPool && NewCompactLoads < 4 && !RangePoolNeeded)
        continue;

      for (const LiteralPoolUse &Use : Pending)
        if (compactLdaWFits(BlockEnd, Use, PlannedPools))
          NextCompactLdaWs.insert(Use.MI);

      PlannedPools.push_back({BlockEnd, PoolBytes});
      NextPoolBeforeBlocks.insert(&*NextMBB);
      Pending.clear();
      PendingPoolEntryIndices.clear();
      PendingPoolEntryCount = 0;
    }

    for (const LiteralPoolUse &Use : Pending)
      if (compactLdaWFits(CodeBytes, Use, PlannedPools))
        NextCompactLdaWs.insert(Use.MI);

    bool Changed = NextCompactLdaWs.size() != CompactLdaWs.size() ||
                   NextPoolBeforeBlocks.size() != PoolBeforeBlocks.size();
    CompactLdaWs = std::move(NextCompactLdaWs);
    PoolBeforeBlocks = std::move(NextPoolBeforeBlocks);
    if (!Changed)
      break;
  }

  HasLdaWPoolPlan = true;
}

static bool shouldUseCompactLdaW(const MachineFunction &MF,
                                 const MachineInstr &LoadMI) {
  uint64_t CodeBytes = 0;
  uint64_t LoadOffset = 0;
  unsigned LoadLiteralIndex = 0;
  unsigned PoolEntryCount = 0;
  bool FoundLoad = false;
  DenseMap<MachineOperand, unsigned> PoolEntryIndices;
  auto GetOrAddPoolEntry = [&](const MachineOperand &MO) {
    auto [It, Inserted] = PoolEntryIndices.try_emplace(MO, PoolEntryCount);
    if (Inserted)
      ++PoolEntryCount;
    return It->second;
  };

  for (const MachineBasicBlock &MBB : MF) {
    for (const MachineInstr &MI : MBB) {
      if (MI.isDebugInstr() || MI.isMetaInstruction())
        continue;

      unsigned Size = 0;
      if (!getKnownInstSize(MF, MI, Size))
        return false;
      if (MI.getOpcode() == AVR32::LDA_W) {
        unsigned LiteralIndex = GetOrAddPoolEntry(MI.getOperand(1));
        if (&MI == &LoadMI) {
          LoadOffset = CodeBytes;
          LoadLiteralIndex = LiteralIndex;
          FoundLoad = true;
        }
      } else if (MI.getOpcode() == AVR32::CALLp) {
        ++PoolEntryCount;
      }
      CodeBytes += Size;
    }
  }

  if (!FoundLoad)
    return false;

  return compactLdaWFits(CodeBytes,
                         {&LoadMI, LoadOffset, LoadLiteralIndex,
                          /*IsLdaW=*/true});
}

INITIALIZE_PASS(AVR32AsmPrinter, "avr32-asm-printer",
                "AVR32 Assembly Printer", false, false)

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVR32AsmPrinter() {
  RegisterAsmPrinter<AVR32AsmPrinter> X(getTheAVR32Target());
}
