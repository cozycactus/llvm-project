//===-- AVR32AsmParser.cpp - Parse AVR32 assembly to MCInst instructions --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../MCTargetDesc/AVR32MCTargetDesc.h"
#include "../TargetInfo/AVR32TargetInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include <string>

using namespace llvm;

#define DEBUG_TYPE "avr32-asm-parser"

namespace {

class AVR32Operand : public MCParsedAsmOperand {
  enum KindTy { Token, Register, Immediate } Kind;
  StringRef Tok{};
  MCRegister Reg;
  const MCExpr *Imm = nullptr;
  SMLoc Start;
  SMLoc End;

public:
  AVR32Operand(StringRef Tok, SMLoc Start)
      : Kind(Token), Tok(Tok), Reg(), Start(Start), End(Start) {}
  AVR32Operand(MCRegister Reg, SMLoc Start, SMLoc End)
      : Kind(Register), Reg(Reg), Start(Start), End(End) {}
  AVR32Operand(const MCExpr *Imm, SMLoc Start, SMLoc End)
      : Kind(Immediate), Reg(), Imm(Imm), Start(Start), End(End) {}

  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isMem() const override { return false; }
  StringRef getToken() const { return Tok; }
  MCRegister getReg() const override {
    assert(Kind == Register && "invalid operand access");
    return Reg;
  }
  const MCExpr *getImm() const {
    assert(Kind == Immediate && "invalid operand access");
    return Imm;
  }
  SMLoc getStartLoc() const override { return Start; }
  SMLoc getEndLoc() const override { return End; }

  bool isSImm8() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isInt<8>(Const->getValue());
  }

  void print(raw_ostream &OS, const MCAsmInfo &MAI) const override {
    if (Kind == Token)
      OS << "Token " << Tok;
    else if (Kind == Register)
      OS << "Register " << Reg.id();
    else
      OS << "Immediate";
  }

  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(Kind == Register && "unexpected operand kind");
    assert(N == 1 && "invalid number of operands");
    Inst.addOperand(MCOperand::createReg(Reg));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(Kind == Immediate && "unexpected operand kind");
    assert(N == 1 && "invalid number of operands");
    if (auto *Const = dyn_cast<MCConstantExpr>(Imm)) {
      Inst.addOperand(MCOperand::createImm(Const->getValue()));
      return;
    }
    Inst.addOperand(MCOperand::createExpr(Imm));
  }

  static std::unique_ptr<AVR32Operand> createToken(StringRef Tok, SMLoc Loc) {
    return std::make_unique<AVR32Operand>(Tok, Loc);
  }

  static std::unique_ptr<AVR32Operand> createReg(MCRegister Reg, SMLoc Start,
                                                 SMLoc End) {
    return std::make_unique<AVR32Operand>(Reg, Start, End);
  }

  static std::unique_ptr<AVR32Operand> createImm(const MCExpr *Imm,
                                                 SMLoc Start, SMLoc End) {
    return std::make_unique<AVR32Operand>(Imm, Start, End);
  }
};

class AVR32AsmParser : public MCTargetAsmParser {
  MCAsmParser &Parser;

#define GET_ASSEMBLER_HEADER
#include "../AVR32GenAsmMatcher.inc"

  bool matchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;

  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                     SMLoc &EndLoc) override;
  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override;

  bool parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;

  ParseStatus parseDirective(AsmToken DirectiveID) override {
    return ParseStatus::NoMatch;
  }

public:
  AVR32AsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                 const MCInstrInfo &MII)
      : MCTargetAsmParser(STI, MII), Parser(Parser) {
    MCAsmParserExtension::Initialize(Parser);
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }

  MCAsmParser &getParser() const { return Parser; }
  AsmLexer &getLexer() const { return Parser.getLexer(); }

private:
  MCRegister parseRegisterName(StringRef Name) const;
  bool parseRegisterOperand(OperandVector &Operands);
  bool parseRegisterCommaRegister(OperandVector &Operands);
  bool parseRegisterOrImmediateOperand(OperandVector &Operands);
};

} // end anonymous namespace

static MCRegister MatchRegisterName(StringRef Name);

bool AVR32AsmParser::matchAndEmitInstruction(SMLoc Loc, unsigned &Opcode,
                                             OperandVector &Operands,
                                             MCStreamer &Out,
                                             uint64_t &ErrorInfo,
                                             bool MatchingInlineAsm) {
  MCInst Inst;
  unsigned MatchResult =
      MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm);

  switch (MatchResult) {
  case Match_Success:
    Inst.setLoc(Loc);
    Out.emitInstruction(Inst, *STI);
    return false;
  case Match_MnemonicFail:
    return Error(Loc, "invalid instruction mnemonic");
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = Loc;
    if (ErrorInfo != ~0U) {
      if (ErrorInfo >= Operands.size())
        return Error(ErrorLoc, "too few operands for instruction");
      ErrorLoc = Operands[ErrorInfo]->getStartLoc();
      if (ErrorLoc == SMLoc())
        ErrorLoc = Loc;
    }
    return Error(ErrorLoc, "invalid operand for instruction");
  }
  case Match_MissingFeature:
    return Error(Loc,
                 "instruction requires a CPU feature not currently enabled");
  default:
    return true;
  }
}

bool AVR32AsmParser::parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                                   SMLoc &EndLoc) {
  ParseStatus Res = tryParseRegister(Reg, StartLoc, EndLoc);
  if (Res.isSuccess())
    return false;
  if (Res.isNoMatch())
    return true;
  return Error(StartLoc, "invalid register name");
}

ParseStatus AVR32AsmParser::tryParseRegister(MCRegister &Reg,
                                             SMLoc &StartLoc, SMLoc &EndLoc) {
  StartLoc = getParser().getTok().getLoc();
  EndLoc = getParser().getTok().getEndLoc();
  if (getLexer().getKind() != AsmToken::Identifier)
    return ParseStatus::NoMatch;

  Reg = parseRegisterName(getParser().getTok().getString());
  if (!Reg)
    return ParseStatus::NoMatch;

  getLexer().Lex();
  return ParseStatus::Success;
}

bool AVR32AsmParser::parseInstruction(ParseInstructionInfo &Info,
                                      StringRef Name, SMLoc NameLoc,
                                      OperandVector &Operands) {
  Operands.push_back(AVR32Operand::createToken(Name, NameLoc));

  if (Name == "add" || Name == "and" || Name == "andn" || Name == "eor" ||
      Name == "or" || Name == "rsub" || Name == "sub") {
    if (parseRegisterCommaRegister(Operands))
      return true;
  } else if (Name == "abs" || Name == "acr" || Name == "brev" ||
             Name == "casts.b" || Name == "casts.h" || Name == "com" ||
             Name == "neg" || Name == "rol" || Name == "ror" ||
             Name == "scr" || Name == "swap.b" || Name == "swap.bh" ||
             Name == "swap.h") {
    if (parseRegisterOperand(Operands))
      return true;
  } else if (Name == "mov") {
    if (parseRegisterOperand(Operands))
      return true;
    if (!parseOptionalToken(AsmToken::Comma))
      return Error(getLexer().getLoc(), "expected comma");
    if (parseRegisterOrImmediateOperand(Operands))
      return true;
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    getParser().eatToEndOfStatement();
    return Error(Loc, "unexpected token in argument list");
  }

  getParser().Lex();
  return false;
}

MCRegister AVR32AsmParser::parseRegisterName(StringRef Name) const {
  std::string LowerStorage = Name.lower();
  StringRef Lower = LowerStorage;
  if (MCRegister Reg = MatchRegisterName(Lower))
    return Reg;

  if (!Lower.consume_front("r"))
    return AVR32::NoRegister;

  unsigned RegNum;
  if (Lower.getAsInteger(10, RegNum))
    return AVR32::NoRegister;

  switch (RegNum) {
  case 13:
    return AVR32::SP;
  case 14:
    return AVR32::LR;
  case 15:
    return AVR32::PC;
  default:
    return AVR32::NoRegister;
  }
}

bool AVR32AsmParser::parseRegisterOperand(OperandVector &Operands) {
  MCRegister Reg;
  SMLoc StartLoc;
  SMLoc EndLoc;
  if (parseRegister(Reg, StartLoc, EndLoc))
    return Error(getLexer().getLoc(), "expected register");
  Operands.push_back(AVR32Operand::createReg(Reg, StartLoc, EndLoc));
  return false;
}

bool AVR32AsmParser::parseRegisterCommaRegister(OperandVector &Operands) {
  if (parseRegisterOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseRegisterOrImmediateOperand(OperandVector &Operands) {
  MCRegister Reg;
  SMLoc StartLoc;
  SMLoc EndLoc;
  ParseStatus RegStatus = tryParseRegister(Reg, StartLoc, EndLoc);
  if (RegStatus.isSuccess()) {
    Operands.push_back(AVR32Operand::createReg(Reg, StartLoc, EndLoc));
    return false;
  }
  if (RegStatus.isFailure())
    return Error(StartLoc, "invalid register name");

  StartLoc = getLexer().getLoc();
  const MCExpr *Expr = nullptr;
  if (getParser().parseExpression(Expr))
    return Error(StartLoc, "expected register or immediate");
  Operands.push_back(
      AVR32Operand::createImm(Expr, StartLoc, getLexer().getLoc()));
  return false;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVR32AsmParser() {
  RegisterMCAsmParser<AVR32AsmParser> X(getTheAVR32Target());
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "../AVR32GenAsmMatcher.inc"
