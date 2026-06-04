//===-- AVR32AsmPrinter.cpp - AVR32 LLVM assembly writer ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32.h"
#include "MCTargetDesc/AVR32InstPrinter.h"
#include "TargetInfo/AVR32TargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {
class AVR32AsmPrinter : public AsmPrinter {
public:
  AVR32AsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  StringRef getPassName() const override { return "AVR32 Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override;
  const MCExpr *lowerSymbolOperand(const MachineOperand &MO);

  static char ID;
};
} // namespace

const MCExpr *AVR32AsmPrinter::lowerSymbolOperand(const MachineOperand &MO) {
  const MCSymbol *Symbol;
  switch (MO.getType()) {
  case MachineOperand::MO_GlobalAddress:
    Symbol = getSymbol(MO.getGlobal());
    break;
  case MachineOperand::MO_ExternalSymbol:
    Symbol = GetExternalSymbolSymbol(MO.getSymbolName());
    break;
  case MachineOperand::MO_MachineBasicBlock:
    Symbol = MO.getMBB()->getSymbol();
    break;
  default:
    llvm_unreachable("Unexpected symbol operand");
  }

  const MCExpr *Expr = MCSymbolRefExpr::create(Symbol, OutContext);
  if (MO.getOffset() != 0)
    Expr = MCBinaryExpr::createAdd(
        Expr, MCConstantExpr::create(MO.getOffset(), OutContext), OutContext);
  return Expr;
}

void AVR32AsmPrinter::emitInstruction(const MachineInstr *MI) {
  MCInst Inst;
  Inst.setOpcode(MI->getOpcode());
  for (const MachineOperand &MO : MI->operands()) {
    if (MO.isReg())
      Inst.addOperand(MCOperand::createReg(MO.getReg()));
    else if (MO.isImm())
      Inst.addOperand(MCOperand::createImm(MO.getImm()));
    else if (MO.isGlobal() || MO.isSymbol() || MO.isMBB())
      Inst.addOperand(MCOperand::createExpr(lowerSymbolOperand(MO)));
    else
      report_fatal_error("AVR32 asm printer cannot lower this operand yet");
  }
  EmitToStreamer(*OutStreamer, Inst);
}

char AVR32AsmPrinter::ID = 0;

INITIALIZE_PASS(AVR32AsmPrinter, "avr32-asm-printer",
                "AVR32 Assembly Printer", false, false)

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVR32AsmPrinter() {
  RegisterAsmPrinter<AVR32AsmPrinter> X(getTheAVR32Target());
}
