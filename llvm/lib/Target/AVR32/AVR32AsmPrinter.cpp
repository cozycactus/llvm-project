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

static bool shouldUseCompactLdaW(const MachineFunction &MF);

namespace {
class AVR32AsmPrinter : public AsmPrinter {
public:
  AVR32AsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  StringRef getPassName() const override { return "AVR32 Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override;
  void emitFunctionBodyEnd() override;
  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       const char *ExtraCode, raw_ostream &O) override;
  const MCExpr *lowerSymbolOperand(const MachineOperand &MO);
  AVR32TargetStreamer &getAVR32TargetStreamer() const;
  void emitLdaWPseudo(const MachineInstr *MI);

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
  default:
    break;
  }

  return true;
}

void AVR32AsmPrinter::emitFunctionBodyEnd() {
  getAVR32TargetStreamer().emitCurrentConstantPool();
}

void AVR32AsmPrinter::emitLdaWPseudo(const MachineInstr *MI) {
  const MachineOperand &Dst = MI->getOperand(0);
  const MachineOperand &Value = MI->getOperand(1);
  assert(Dst.isReg() && "lda.w destination must be a register");

  const MCExpr *ValueExpr = nullptr;
  if (Value.isGlobal() || Value.isSymbol() || Value.isMBB())
    ValueExpr = lowerSymbolOperand(Value);
  else if (Value.isImm())
    ValueExpr = MCConstantExpr::create(Value.getImm(), OutContext);
  else
    report_fatal_error("AVR32 lda.w cannot lower this operand yet");

  const MCExpr *CPLoc =
      getAVR32TargetStreamer().addCPENTConstantPoolEntry(ValueExpr, SMLoc());

  MCInst Load;
  Load.setOpcode(shouldUseCompactLdaW(*MF) ? AVR32::LDDPCcp
                                           : AVR32::LDDPC_PCREL16);
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
    else if (MO.isGlobal() || MO.isSymbol() || MO.isMBB())
      Inst.addOperand(MCOperand::createExpr(lowerSymbolOperand(MO)));
    else if (MO.isRegMask())
      continue;
    else
      report_fatal_error("AVR32 asm printer cannot lower this operand yet");
  }
  EmitToStreamer(*OutStreamer, Inst);
}

char AVR32AsmPrinter::ID = 0;

static bool shouldUseCompactLdaW(const MachineFunction &MF) {
  uint64_t CodeBytes = 0;
  unsigned LiteralLoads = 0;

  for (const MachineBasicBlock &MBB : MF) {
    for (const MachineInstr &MI : MBB) {
      if (MI.isInlineAsm())
        return false;
      if (MI.getOpcode() == AVR32::LDA_W)
        ++LiteralLoads;
      if (MI.isDebugInstr() || MI.isMetaInstruction())
        continue;

      unsigned Size = MI.getDesc().getSize();
      if (Size == 0)
        return false;
      CodeBytes += Size;
    }
  }

  if (LiteralLoads == 0)
    return false;

  uint64_t AlignedCodeBytes = (CodeBytes + 3) & ~uint64_t(3);
  return AlignedCodeBytes + uint64_t(LiteralLoads) * 4 <= 508;
}

INITIALIZE_PASS(AVR32AsmPrinter, "avr32-asm-printer",
                "AVR32 Assembly Printer", false, false)

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVR32AsmPrinter() {
  RegisterAsmPrinter<AVR32AsmPrinter> X(getTheAVR32Target());
}
