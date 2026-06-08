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
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"

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
  const MCExpr *lowerSymbolOperand(const MachineOperand &MO);
  AVR32TargetStreamer &getAVR32TargetStreamer() const;
  void buildLdaWPoolPlan(const MachineFunction &MF);
  void emitLdaWPseudo(const MachineInstr *MI);
  void emitPendingConstantPool();

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
  case MachineOperand::MO_JumpTableIndex:
    O << *GetJTISymbol(MO.getIndex());
    return false;
  default:
    break;
  }

  return true;
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
  if (Value.isGlobal() || Value.isSymbol() || Value.isMBB() || Value.isJTI())
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

void AVR32AsmPrinter::emitInstruction(const MachineInstr *MI) {
  if (MI->getOpcode() == AVR32::LDA_W)
    return emitLdaWPseudo(MI);

  MCInst Inst;
  Inst.setOpcode(MI->getOpcode());
  for (const MachineOperand &MO : MI->operands()) {
    if (MO.isReg())
      Inst.addOperand(MCOperand::createReg(MO.getReg()));
    else if (MO.isImm())
      Inst.addOperand(MCOperand::createImm(MO.getImm()));
    else if (MO.isGlobal() || MO.isSymbol() || MO.isMBB() || MO.isJTI())
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

static bool isCompactBranchOpcode(unsigned Opcode) {
  switch (Opcode) {
  case AVR32::RJMPbb:
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

static bool compactBranchCrossesBoundary(
    const MachineFunction &MF,
    const DenseMap<const MachineBasicBlock *, uint64_t> &BlockOffsets,
    const DenseMap<const MachineInstr *, uint64_t> &InstrOffsets,
    uint64_t BoundaryOffset) {
  for (const MachineBasicBlock &MBB : MF) {
    for (const MachineInstr &MI : MBB) {
      if (!isCompactBranchOpcode(MI.getOpcode()) || MI.getNumOperands() != 1 ||
          !MI.getOperand(0).isMBB())
        continue;

      uint64_t BranchOffset = InstrOffsets.lookup(&MI);
      uint64_t TargetOffset = BlockOffsets.lookup(MI.getOperand(0).getMBB());
      if ((BranchOffset < BoundaryOffset && BoundaryOffset <= TargetOffset) ||
          (TargetOffset < BoundaryOffset && BoundaryOffset <= BranchOffset))
        return true;
    }
  }
  return false;
}

struct LdaWUse {
  const MachineInstr *MI;
  uint64_t Offset;
};

static bool compactLdaWFits(uint64_t PoolOffset, const LdaWUse &Use,
                            unsigned LiteralIndex) {
  uint64_t MaxDistance =
      align4(PoolOffset) - Use.Offset + uint64_t(LiteralIndex + 1) * 4;
  return MaxDistance <= 508;
}

void AVR32AsmPrinter::buildLdaWPoolPlan(const MachineFunction &MF) {
  CompactLdaWs.clear();
  PoolBeforeBlocks.clear();
  HasLdaWPoolPlan = false;
  PendingLdaW = 0;

  for (unsigned Iter = 0; Iter != 8; ++Iter) {
    DenseMap<const MachineBasicBlock *, uint64_t> BlockOffsets;
    DenseMap<const MachineInstr *, uint64_t> InstrOffsets;
    SmallVector<LdaWUse, 64> AllLoads;
    uint64_t CodeBytes = 0;

    for (const MachineBasicBlock &MBB : MF) {
      BlockOffsets[&MBB] = CodeBytes;
      for (const MachineInstr &MI : MBB) {
        if (MI.isInlineAsm())
          return;
        if (MI.isDebugInstr() || MI.isMetaInstruction())
          continue;

        unsigned Size = MI.getDesc().getSize();
        if (Size == 0)
          return;
        if (MI.getOpcode() == AVR32::LDA_W && CompactLdaWs.contains(&MI))
          Size = 2;

        InstrOffsets[&MI] = CodeBytes;
        if (MI.getOpcode() == AVR32::LDA_W)
          AllLoads.push_back({&MI, CodeBytes});
        CodeBytes += Size;
      }
    }

    DenseSet<const MachineInstr *> NextCompactLdaWs = CompactLdaWs;
    DenseSet<const MachineBasicBlock *> NextPoolBeforeBlocks =
        PoolBeforeBlocks;

    for (auto [Index, Use] : enumerate(AllLoads))
      if (compactLdaWFits(CodeBytes, Use, Index))
        NextCompactLdaWs.insert(Use.MI);

    SmallVector<LdaWUse, 32> Pending;
    unsigned NextLoad = 0;

    for (const MachineBasicBlock &MBB : MF) {
      uint64_t BlockEnd = BlockOffsets.lookup(&MBB);
      for (const MachineInstr &MI : MBB) {
        if (MI.isDebugInstr() || MI.isMetaInstruction())
          continue;
        if (MI.getOpcode() == AVR32::LDA_W)
          Pending.push_back(AllLoads[NextLoad++]);
        unsigned Size = MI.getDesc().getSize();
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

      if (compactBranchCrossesBoundary(MF, BlockOffsets, InstrOffsets, BlockEnd))
        continue;

      unsigned NewCompactLoads = 0;
      for (auto [Index, Use] : enumerate(Pending))
        if (!CompactLdaWs.contains(Use.MI) &&
            compactLdaWFits(BlockEnd, Use, Index))
          ++NewCompactLoads;

      bool KeepExistingPool = PoolBeforeBlocks.contains(&*NextMBB);
      if (!KeepExistingPool && NewCompactLoads < 4)
        continue;

      for (auto [Index, Use] : enumerate(Pending))
        if (compactLdaWFits(BlockEnd, Use, Index))
          NextCompactLdaWs.insert(Use.MI);

      NextPoolBeforeBlocks.insert(&*NextMBB);
      Pending.clear();
    }

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
  unsigned LiteralLoads = 0;
  uint64_t LoadOffset = 0;
  unsigned LoadLiteralIndex = 0;
  bool FoundLoad = false;

  for (const MachineBasicBlock &MBB : MF) {
    for (const MachineInstr &MI : MBB) {
      if (MI.isInlineAsm())
        return false;
      if (MI.isDebugInstr() || MI.isMetaInstruction())
        continue;

      unsigned Size = MI.getDesc().getSize();
      if (Size == 0)
        return false;
      if (&MI == &LoadMI) {
        LoadOffset = CodeBytes;
        LoadLiteralIndex = LiteralLoads;
        FoundLoad = true;
      }
      if (MI.getOpcode() == AVR32::LDA_W)
        ++LiteralLoads;
      CodeBytes += Size;
    }
  }

  if (!FoundLoad)
    return false;

  return compactLdaWFits(CodeBytes, {&LoadMI, LoadOffset}, LoadLiteralIndex);
}

INITIALIZE_PASS(AVR32AsmPrinter, "avr32-asm-printer",
                "AVR32 Assembly Printer", false, false)

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVR32AsmPrinter() {
  RegisterAsmPrinter<AVR32AsmPrinter> X(getTheAVR32Target());
}
