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
#include <string>

using namespace llvm;

#define DEBUG_TYPE "avr32-asm-parser"

namespace {

class AVR32Operand : public MCParsedAsmOperand {
  enum KindTy { Token, Register } Kind;
  StringRef Tok{};
  MCRegister Reg;
  SMLoc Start;
  SMLoc End;

public:
  AVR32Operand(StringRef Tok, SMLoc Start)
      : Kind(Token), Tok(Tok), Reg(), Start(Start), End(Start) {}
  AVR32Operand(MCRegister Reg, SMLoc Start, SMLoc End)
      : Kind(Register), Reg(Reg), Start(Start), End(End) {}

  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return false; }
  bool isMem() const override { return false; }
  StringRef getToken() const { return Tok; }
  MCRegister getReg() const override {
    assert(Kind == Register && "invalid operand access");
    return Reg;
  }
  SMLoc getStartLoc() const override { return Start; }
  SMLoc getEndLoc() const override { return End; }

  void print(raw_ostream &OS, const MCAsmInfo &MAI) const override {
    if (Kind == Token)
      OS << "Token " << Tok;
    else
      OS << "Register " << Reg.id();
  }

  static std::unique_ptr<AVR32Operand> createToken(StringRef Tok, SMLoc Loc) {
    return std::make_unique<AVR32Operand>(Tok, Loc);
  }

  static std::unique_ptr<AVR32Operand> createReg(MCRegister Reg, SMLoc Start,
                                                 SMLoc End) {
    return std::make_unique<AVR32Operand>(Reg, Start, End);
  }
};

class AVR32AsmParser : public MCTargetAsmParser {
  MCAsmParser &Parser;

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

  void convertToMapAndConstraints(unsigned Kind,
                                  const OperandVector &Operands) override {}

public:
  AVR32AsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                 const MCInstrInfo &MII)
      : MCTargetAsmParser(STI, MII), Parser(Parser) {
    MCAsmParserExtension::Initialize(Parser);
  }

  MCAsmParser &getParser() const { return Parser; }
  AsmLexer &getLexer() const { return Parser.getLexer(); }

private:
  MCRegister parseRegisterName(StringRef Name) const;
  bool parseRegisterOperand(OperandVector &Operands);
};

} // end anonymous namespace

bool AVR32AsmParser::matchAndEmitInstruction(SMLoc Loc, unsigned &Opcode,
                                             OperandVector &Operands,
                                             MCStreamer &Out,
                                             uint64_t &ErrorInfo,
                                             bool MatchingInlineAsm) {
  assert(!Operands.empty() && "missing AVR32 mnemonic operand");
  const auto &Token = static_cast<const AVR32Operand &>(*Operands[0]);

  MCInst Inst;
  if (Token.getToken() == "nop") {
    if (Operands.size() != 1)
      return Error(Loc, "invalid operand for instruction");
    Inst.setOpcode(AVR32::NOP);
  } else if (Token.getToken() == "mov") {
    if (Operands.size() != 3)
      return Error(Loc, "invalid operand for instruction");
    const auto &Rd = static_cast<const AVR32Operand &>(*Operands[1]);
    const auto &Rs = static_cast<const AVR32Operand &>(*Operands[2]);
    if (!Rd.isReg() || !Rs.isReg())
      return Error(Loc, "invalid operand for instruction");
    Inst.setOpcode(AVR32::MOVrr);
    Inst.addOperand(MCOperand::createReg(Rd.getReg()));
    Inst.addOperand(MCOperand::createReg(Rs.getReg()));
  } else {
    return Error(Loc, "invalid instruction mnemonic");
  }

  Inst.setLoc(Loc);
  Out.emitInstruction(Inst, *STI);
  return false;
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

  if (Name == "mov") {
    if (parseRegisterOperand(Operands))
      return true;
    if (!parseOptionalToken(AsmToken::Comma))
      return Error(getLexer().getLoc(), "expected comma");
    if (parseRegisterOperand(Operands))
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
  if (Lower == "sp")
    return AVR32::SP;
  if (Lower == "lr")
    return AVR32::LR;
  if (Lower == "pc")
    return AVR32::PC;

  if (!Lower.consume_front("r"))
    return AVR32::NoRegister;

  unsigned RegNum;
  if (Lower.getAsInteger(10, RegNum) || RegNum > 15)
    return AVR32::NoRegister;

  switch (RegNum) {
  case 0:
    return AVR32::R0;
  case 1:
    return AVR32::R1;
  case 2:
    return AVR32::R2;
  case 3:
    return AVR32::R3;
  case 4:
    return AVR32::R4;
  case 5:
    return AVR32::R5;
  case 6:
    return AVR32::R6;
  case 7:
    return AVR32::R7;
  case 8:
    return AVR32::R8;
  case 9:
    return AVR32::R9;
  case 10:
    return AVR32::R10;
  case 11:
    return AVR32::R11;
  case 12:
    return AVR32::R12;
  case 13:
    return AVR32::SP;
  case 14:
    return AVR32::LR;
  case 15:
    return AVR32::PC;
  default:
    llvm_unreachable("invalid AVR32 register number");
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

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVR32AsmParser() {
  RegisterMCAsmParser<AVR32AsmParser> X(getTheAVR32Target());
}
