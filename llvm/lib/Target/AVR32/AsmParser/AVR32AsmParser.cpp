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

  bool isSImm16() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isInt<16>(Const->getValue());
  }

  bool isSImm21() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isInt<21>(Const->getValue());
  }

  bool isSImm12Shift1() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= -4096 &&
           Const->getValue() <= 4094 && Const->getValue() % 2 == 0;
  }

  bool isSImm12Shift2() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= -8192 &&
           Const->getValue() <= 8188 && Const->getValue() % 4 == 0;
  }

  bool isSImm15Shift2() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= -65536 &&
           Const->getValue() <= 65532 && Const->getValue() % 4 == 0;
  }

  bool isSubSPImm() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= -512 && Const->getValue() <= 508 &&
           Const->getValue() % 4 == 0;
  }

  bool isSImm6() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isInt<6>(Const->getValue());
  }

  bool isSImm11() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isInt<11>(Const->getValue());
  }

  bool isSImm12() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isInt<12>(Const->getValue());
  }

  bool isUImm5() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isUInt<5>(Const->getValue());
  }

  bool isUImm2() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isUInt<2>(Const->getValue());
  }

  bool isUImm3() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isUInt<3>(Const->getValue());
  }

  bool isUImm3Shift1() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= 0 && Const->getValue() <= 14 &&
           Const->getValue() % 2 == 0;
  }

  bool isUImm4Shift2() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= 0 && Const->getValue() <= 60 &&
           Const->getValue() % 4 == 0;
  }

  bool isUImm5Shift2() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= 0 && Const->getValue() <= 124 &&
           Const->getValue() % 4 == 0;
  }

  bool isUImm7Shift2() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= 0 && Const->getValue() <= 508 &&
           Const->getValue() % 4 == 0;
  }

  bool isUImm8() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isUInt<8>(Const->getValue());
  }

  bool isUImm8Shift2() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= 0 && Const->getValue() <= 1020 &&
           Const->getValue() % 4 == 0;
  }

  bool isUImm9() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isUInt<9>(Const->getValue());
  }

  bool isUImm9Shift1() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= 0 && Const->getValue() <= 1022 &&
           Const->getValue() % 2 == 0;
  }

  bool isUImm9Shift2() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= 0 && Const->getValue() <= 2044 &&
           Const->getValue() % 4 == 0;
  }

  bool isUImm16() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isUInt<16>(Const->getValue());
  }

  bool isACallDisp() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= 0 && Const->getValue() <= 1020 &&
           Const->getValue() % 4 == 0;
  }

  bool isSysRegAddr() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= 0 && Const->getValue() <= 1020 &&
           Const->getValue() % 4 == 0;
  }

  bool isDebugRegAddr() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= 0 && Const->getValue() <= 1020 &&
           Const->getValue() % 4 == 0;
  }

  bool isIncJOSPImm() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() >= -4 && Const->getValue() <= 4 &&
           Const->getValue() != 0;
  }

  bool isHalfPart() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && (Const->getValue() == 0 || Const->getValue() == 1);
  }

  bool isBytePart() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && isUInt<2>(Const->getValue());
  }

  bool isFixedShift2() const {
    if (Kind != Immediate)
      return false;
    auto *Const = dyn_cast<MCConstantExpr>(Imm);
    return Const && Const->getValue() == 2;
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
  bool parseImmediateOperand(OperandVector &Operands);
  bool parseRegisterCommaRegister(OperandVector &Operands);
  bool parseRegisterCommaRegisterCommaRegister(OperandVector &Operands);
  bool parseRegisterCommaRegisterCommaRegisterOrImmediate(
      OperandVector &Operands);
  bool parseRegisterCommaRegisterCommaRegisterOrImmediateOperand(
      OperandVector &Operands);
  bool parseRegisterShiftRightImmediateCommaImmediate(OperandVector &Operands);
  bool parseSubOperands(
      OperandVector &Operands);
  bool parseRegisterHalfPart(OperandVector &Operands);
  bool parseHalfPartOperand(OperandVector &Operands);
  bool parseBytePartOperand(OperandVector &Operands);
  bool parseRegisterCommaRegisterHalfPartCommaRegisterHalfPart(
      OperandVector &Operands);
  bool parseLoadInsertOperands(OperandVector &Operands, bool IsByte);
  bool parseLoadOperands(OperandVector &Operands,
                         bool AllowBareRegister = false);
  bool parseStoreHalfwordPairOperands(OperandVector &Operands);
  bool parseStoreByteOperands(OperandVector &Operands);
  bool parseStoreDoubleOperands(OperandVector &Operands);
  bool parseMemoryDispOperand(OperandVector &Operands);
  bool parseMemoryDispCommaImmediate(OperandVector &Operands);
  bool parseRegisterCommaImmediate(OperandVector &Operands);
  bool parseRegisterCommaImmediateOptionalCOH(OperandVector &Operands);
  bool parseRegisterCommaRegisterOrImmediate(OperandVector &Operands);
  bool parseImmediateCommaRegister(OperandVector &Operands);
  bool parseImmediateCommaImmediate(OperandVector &Operands);
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

  if (Name == "adc" || Name == "addabs" ||
      Name == "addal" || Name == "addcc" || Name == "addcs" ||
      Name == "addeq" || Name == "addge" || Name == "addgt" ||
      Name == "addhi" || Name == "addhs" || Name == "addle" ||
      Name == "addlo" || Name == "addls" || Name == "addlt" ||
      Name == "addmi" || Name == "addne" || Name == "addpl" ||
      Name == "addqs" || Name == "addvc" || Name == "addvs" ||
      Name == "andal" || Name == "andcc" || Name == "andcs" ||
      Name == "andeq" || Name == "andge" || Name == "andgt" ||
      Name == "andhi" || Name == "andhs" || Name == "andle" ||
      Name == "andlo" || Name == "andls" || Name == "andlt" ||
      Name == "andmi" || Name == "andne" || Name == "andpl" ||
      Name == "andqs" || Name == "andvc" || Name == "andvs" ||
      Name == "asr" ||
      Name == "divs" || Name == "divu" ||
      Name == "eoral" || Name == "eorcc" || Name == "eorcs" ||
      Name == "eoreq" || Name == "eorge" || Name == "eorgt" ||
      Name == "eorhi" || Name == "eorhs" || Name == "eorle" ||
      Name == "eorlo" || Name == "eorls" || Name == "eorlt" ||
      Name == "eormi" || Name == "eorne" || Name == "eorpl" ||
      Name == "eorqs" || Name == "eorvc" || Name == "eorvs" ||
      Name == "lsl" ||
      Name == "lsr" || Name == "max" || Name == "min" ||
      Name == "oral" || Name == "orcc" || Name == "orcs" ||
      Name == "oreq" || Name == "orge" || Name == "orgt" ||
      Name == "orhi" || Name == "orhs" || Name == "orle" ||
      Name == "orlo" || Name == "orls" || Name == "orlt" ||
      Name == "ormi" || Name == "orne" || Name == "orpl" ||
      Name == "orqs" || Name == "orvc" || Name == "orvs" ||
      Name == "muls.d" || Name == "mulu.d" ||
      Name == "psad" || Name == "psub.b" || Name == "psub.h" ||
      Name == "satadd.w" ||
      Name == "satsub.h" || Name == "sbc" || Name == "xchg") {
    if (parseRegisterCommaRegisterCommaRegister(Operands))
      return true;
  } else if (Name == "satsub.w") {
    if (parseRegisterCommaRegisterCommaRegisterOrImmediateOperand(Operands))
      return true;
  } else if (Name == "satrnds" || Name == "satrndu" || Name == "sats" ||
             Name == "satu") {
    if (parseRegisterShiftRightImmediateCommaImmediate(Operands))
      return true;
  } else if (Name == "subal" || Name == "subcc" || Name == "subcs" ||
             Name == "subeq" || Name == "subge" || Name == "subgt" ||
             Name == "subhi" || Name == "subhs" || Name == "suble" ||
             Name == "sublo" || Name == "subls" || Name == "sublt" ||
             Name == "submi" || Name == "subne" || Name == "subpl" ||
             Name == "subqs" || Name == "subvc" || Name == "subvs") {
    if (parseRegisterCommaRegisterCommaRegisterOrImmediate(Operands))
      return true;
  } else if (Name == "mul") {
    if (parseRegisterCommaRegister(Operands))
      return true;
    if (parseOptionalToken(AsmToken::Comma) &&
        parseRegisterOrImmediateOperand(Operands))
      return true;
  } else if (Name == "add" || Name == "and" || Name == "andn" ||
             Name == "clz" || Name == "cp.b" || Name == "cp.h" ||
             Name == "eor" || Name == "or" || Name == "rsub" ||
             Name == "tst") {
    if (parseRegisterCommaRegister(Operands))
      return true;
  } else if (Name == "sub") {
    if (parseSubOperands(Operands))
      return true;
  } else if (Name == "subhh.w") {
    if (parseRegisterCommaRegisterHalfPartCommaRegisterHalfPart(Operands))
      return true;
  } else if (Name == "moval" || Name == "movcc" || Name == "movcs" ||
             Name == "moveq" || Name == "movge" || Name == "movgt" ||
             Name == "movhi" || Name == "movhs" || Name == "movle" ||
             Name == "movlo" || Name == "movls" || Name == "movlt" ||
             Name == "movmi" || Name == "movne" || Name == "movpl" ||
             Name == "movqs" || Name == "movvc" || Name == "movvs") {
    if (parseRegisterCommaRegisterOrImmediate(Operands))
      return true;
  } else if (Name == "andh" || Name == "andl") {
    if (parseRegisterCommaImmediateOptionalCOH(Operands))
      return true;
  } else if (Name == "bld" || Name == "bst" || Name == "cbr" ||
             Name == "sbr") {
    if (parseRegisterCommaImmediate(Operands))
      return true;
  } else if (Name == "rsubal" || Name == "rsubcc" || Name == "rsubcs" ||
             Name == "rsubeq" || Name == "rsubge" || Name == "rsubgt" ||
             Name == "rsubhi" || Name == "rsubhs" || Name == "rsuble" ||
             Name == "rsublo" || Name == "rsubls" || Name == "rsublt" ||
             Name == "rsubmi" || Name == "rsubne" || Name == "rsubpl" ||
             Name == "rsubqs" || Name == "rsubvc" || Name == "rsubvs" ||
             Name == "subfal" || Name == "subfcc" || Name == "subfcs" ||
             Name == "subfeq" || Name == "subfge" || Name == "subfgt" ||
             Name == "subfhi" || Name == "subfhs" || Name == "subfle" ||
             Name == "subflo" || Name == "subfls" || Name == "subflt" ||
             Name == "subfmi" || Name == "subfne" || Name == "subfpl" ||
             Name == "subfqs" || Name == "subfvc" || Name == "subfvs") {
    if (parseRegisterCommaImmediate(Operands))
      return true;
  } else if (Name == "cp.w") {
    if (parseRegisterOperand(Operands))
      return true;
    if (!parseOptionalToken(AsmToken::Comma))
      return Error(getLexer().getLoc(), "expected comma");
    if (parseRegisterOrImmediateOperand(Operands))
      return true;
  } else if (Name == "cpc") {
    if (parseRegisterOperand(Operands))
      return true;
    if (parseOptionalToken(AsmToken::Comma) &&
        parseRegisterOperand(Operands))
      return true;
  } else if (Name == "acall" || Name == "csrf" || Name == "csrfcz" ||
             Name == "ssrf") {
    if (parseImmediateOperand(Operands))
      return true;
  } else if (Name == "incjosp" || Name == "sleep" || Name == "sync") {
    if (parseImmediateOperand(Operands))
      return true;
  } else if (Name == "memc" || Name == "mems") {
    if (parseImmediateCommaImmediate(Operands))
      return true;
  } else if (Name == "ldins.b" || Name == "ldins.h") {
    if (parseLoadInsertOperands(Operands, Name == "ldins.b"))
      return true;
  } else if (Name == "ld.d" || Name == "lddpc" || Name == "lddsp" ||
             Name == "ld.sb" || Name == "ld.sbal" || Name == "ld.sbcc" ||
             Name == "ld.sbcs" || Name == "ld.sbeq" || Name == "ld.sbge" ||
             Name == "ld.sbgt" || Name == "ld.sbhi" || Name == "ld.sbhs" ||
             Name == "ld.sble" || Name == "ld.sblo" || Name == "ld.sbls" ||
             Name == "ld.sblt" || Name == "ld.sbmi" || Name == "ld.sbne" ||
             Name == "ld.sbpl" || Name == "ld.sbqs" || Name == "ld.sbvc" ||
             Name == "ld.sbvs" ||
             Name == "ld.sh" || Name == "ld.shal" || Name == "ld.shcc" ||
             Name == "ld.shcs" || Name == "ld.sheq" || Name == "ld.shge" ||
             Name == "ld.shgt" || Name == "ld.shhi" || Name == "ld.shhs" ||
             Name == "ld.shle" || Name == "ld.shlo" || Name == "ld.shls" ||
             Name == "ld.shlt" || Name == "ld.shmi" || Name == "ld.shne" ||
             Name == "ld.shpl" || Name == "ld.shqs" || Name == "ld.shvc" ||
             Name == "ld.shvs" ||
             Name == "ld.uh" || Name == "ld.uhal" || Name == "ld.uhcc" ||
             Name == "ld.uhcs" || Name == "ld.uheq" || Name == "ld.uhge" ||
             Name == "ld.uhgt" || Name == "ld.uhhi" || Name == "ld.uhhs" ||
             Name == "ld.uhle" || Name == "ld.uhlo" || Name == "ld.uhls" ||
             Name == "ld.uhlt" || Name == "ld.uhmi" || Name == "ld.uhne" ||
             Name == "ld.uhpl" || Name == "ld.uhqs" || Name == "ld.uhvc" ||
             Name == "ld.uhvs" ||
             Name == "ld.ub" || Name == "ld.ubal" || Name == "ld.ubcc" ||
             Name == "ld.ubcs" || Name == "ld.ubeq" || Name == "ld.ubge" ||
             Name == "ld.ubgt" || Name == "ld.ubhi" || Name == "ld.ubhs" ||
             Name == "ld.uble" || Name == "ld.ublo" || Name == "ld.ubls" ||
             Name == "ld.ublt" || Name == "ld.ubmi" || Name == "ld.ubne" ||
             Name == "ld.ubpl" || Name == "ld.ubqs" || Name == "ld.ubvc" ||
             Name == "ld.ubvs" ||
             Name == "ld.w" || Name == "ld.wal" || Name == "ld.wcc" ||
             Name == "ld.wcs" || Name == "ld.weq" || Name == "ld.wge" ||
             Name == "ld.wgt" || Name == "ld.whi" || Name == "ld.whs" ||
             Name == "ld.wle" || Name == "ld.wlo" || Name == "ld.wls" ||
             Name == "ld.wlt" || Name == "ld.wmi" || Name == "ld.wne" ||
             Name == "ld.wpl" || Name == "ld.wqs" || Name == "ld.wvc" ||
             Name == "ld.wvs") {
    if (parseLoadOperands(Operands, Name == "ld.d"))
      return true;
  } else if (Name == "st.b" || Name == "st.bal" || Name == "st.bcc" ||
             Name == "st.bcs" || Name == "st.beq" || Name == "st.bge" ||
             Name == "st.bgt" || Name == "st.bhi" || Name == "st.bhs" ||
             Name == "st.ble" || Name == "st.blo" || Name == "st.bls" ||
             Name == "st.blt" || Name == "st.bmi" || Name == "st.bne" ||
             Name == "st.bpl" || Name == "st.bqs" || Name == "st.bvc" ||
             Name == "st.bvs" || Name == "st.h" || Name == "st.hal" ||
             Name == "st.hcc" || Name == "st.hcs" || Name == "st.heq" ||
             Name == "st.hge" || Name == "st.hgt" || Name == "st.hhi" ||
             Name == "st.hhs" || Name == "st.hle" || Name == "st.hlo" ||
             Name == "st.hls" || Name == "st.hlt" || Name == "st.hmi" ||
             Name == "st.hne" || Name == "st.hpl" || Name == "st.hqs" ||
             Name == "st.hvc" || Name == "st.hvs" || Name == "st.w" ||
             Name == "st.wal" || Name == "st.wcc" || Name == "st.wcs" ||
             Name == "st.weq" || Name == "st.wge" || Name == "st.wgt" ||
             Name == "st.whi" || Name == "st.whs" || Name == "st.wle" ||
             Name == "st.wlo" || Name == "st.wls" || Name == "st.wlt" ||
             Name == "st.wmi" || Name == "st.wne" || Name == "st.wpl" ||
             Name == "st.wqs" || Name == "st.wvc" || Name == "st.wvs" ||
             Name == "stcond" || Name == "stdsp" || Name == "stswp.h" ||
             Name == "stswp.w") {
    if (parseStoreByteOperands(Operands))
      return true;
  } else if (Name == "sthh.w") {
    if (parseStoreHalfwordPairOperands(Operands))
      return true;
  } else if (Name == "st.d") {
    if (parseStoreDoubleOperands(Operands))
      return true;
  } else if (Name == "eorh" || Name == "eorl" || Name == "mfdr" ||
             Name == "mfsr" || Name == "movh" || Name == "orh" ||
             Name == "orl") {
    if (parseRegisterCommaImmediate(Operands))
      return true;
  } else if (Name == "mtdr" || Name == "mtsr") {
    if (parseImmediateCommaRegister(Operands))
      return true;
  } else if (Name == "cache") {
    if (parseMemoryDispCommaImmediate(Operands))
      return true;
  } else if (Name == "pref") {
    if (parseMemoryDispOperand(Operands))
      return true;
  } else if (Name == "abs" || Name == "acr" || Name == "brev" ||
             Name == "casts.b" || Name == "casts.h" || Name == "castu.h" ||
             Name == "castu.b" || Name == "com" || Name == "icall" ||
             Name == "musfr" || Name == "mustr" ||
             Name == "neg" || Name == "retal" || Name == "retcc" ||
             Name == "retcs" || Name == "reteq" || Name == "reths" ||
             Name == "retge" || Name == "retgt" || Name == "rethi" ||
             Name == "retlo" || Name == "retle" || Name == "retlt" ||
             Name == "retls" ||
             Name == "retmi" || Name == "retne" || Name == "retpl" ||
             Name == "retqs" || Name == "retvc" || Name == "retvs" ||
             Name == "rol" || Name == "ror" || Name == "scr" ||
             Name == "sral" || Name == "srcc" || Name == "srcs" ||
             Name == "sreq" || Name == "srge" || Name == "srgt" ||
             Name == "srhi" || Name == "srhs" || Name == "srle" ||
             Name == "srlo" || Name == "srls" || Name == "srlt" ||
             Name == "srmi" || Name == "srne" || Name == "srpl" ||
             Name == "srqs" || Name == "srvc" || Name == "srvs" ||
             Name == "swap.b" || Name == "swap.bh" || Name == "swap.h" ||
             Name == "tnbz") {
    if (parseRegisterOperand(Operands))
      return true;
  } else if (Name == "mov") {
    if (parseRegisterCommaRegisterOrImmediate(Operands))
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

bool AVR32AsmParser::parseImmediateOperand(OperandVector &Operands) {
  SMLoc StartLoc = getLexer().getLoc();
  const MCExpr *Expr = nullptr;
  if (getParser().parseExpression(Expr))
    return Error(StartLoc, "expected immediate");
  Operands.push_back(
      AVR32Operand::createImm(Expr, StartLoc, getLexer().getLoc()));
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

bool AVR32AsmParser::parseRegisterCommaRegisterCommaRegister(
    OperandVector &Operands) {
  if (parseRegisterCommaRegister(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseRegisterCommaRegisterCommaRegisterOrImmediate(
    OperandVector &Operands) {
  if (parseRegisterOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");

  MCRegister Reg;
  SMLoc StartLoc;
  SMLoc EndLoc;
  ParseStatus RegStatus = tryParseRegister(Reg, StartLoc, EndLoc);
  if (RegStatus.isSuccess()) {
    Operands.push_back(AVR32Operand::createReg(Reg, StartLoc, EndLoc));
    if (!parseOptionalToken(AsmToken::Comma))
      return Error(getLexer().getLoc(), "expected comma");
    if (parseRegisterOperand(Operands))
      return true;
    return false;
  }
  if (RegStatus.isFailure())
    return Error(StartLoc, "invalid register name");

  if (parseImmediateOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseRegisterCommaRegisterCommaRegisterOrImmediateOperand(
    OperandVector &Operands) {
  if (parseRegisterCommaRegister(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterOrImmediateOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseRegisterShiftRightImmediateCommaImmediate(
    OperandVector &Operands) {
  if (parseRegisterOperand(Operands))
    return true;
  if (getLexer().isNot(AsmToken::GreaterGreater))
    return Error(getLexer().getLoc(), "expected >>");
  Operands.push_back(AVR32Operand::createToken(">>", getLexer().getLoc()));
  getLexer().Lex();
  if (parseImmediateOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseImmediateOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseSubOperands(OperandVector &Operands) {
  if (parseRegisterOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterOrImmediateOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return false;

  MCRegister Reg;
  SMLoc StartLoc;
  SMLoc EndLoc;
  ParseStatus RegStatus = tryParseRegister(Reg, StartLoc, EndLoc);
  if (RegStatus.isSuccess()) {
    Operands.push_back(AVR32Operand::createReg(Reg, StartLoc, EndLoc));
    if (getLexer().isNot(AsmToken::LessLess))
      return Error(getLexer().getLoc(), "expected <<");
    Operands.push_back(AVR32Operand::createToken("<<", getLexer().getLoc()));
    getLexer().Lex();
    if (parseImmediateOperand(Operands))
      return true;
    return false;
  }
  if (RegStatus.isFailure())
    return Error(StartLoc, "invalid register name");

  if (parseImmediateOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseRegisterHalfPart(OperandVector &Operands) {
  if (parseRegisterOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Colon))
    return Error(getLexer().getLoc(), "expected :");
  Operands.push_back(AVR32Operand::createToken(":", getLexer().getLoc()));

  if (parseHalfPartOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseHalfPartOperand(OperandVector &Operands) {
  SMLoc PartLoc = getLexer().getLoc();
  if (getLexer().isNot(AsmToken::Identifier))
    return Error(PartLoc, "expected b or t");
  StringRef Part = getLexer().getTok().getString();
  int64_t PartValue;
  if (Part == "b")
    PartValue = 0;
  else if (Part == "t")
    PartValue = 1;
  else
    return Error(PartLoc, "expected b or t");

  const MCExpr *PartExpr = MCConstantExpr::create(PartValue, getContext());
  getLexer().Lex();
  Operands.push_back(
      AVR32Operand::createImm(PartExpr, PartLoc, getLexer().getLoc()));
  return false;
}

bool AVR32AsmParser::parseBytePartOperand(OperandVector &Operands) {
  SMLoc PartLoc = getLexer().getLoc();
  if (getLexer().isNot(AsmToken::Identifier))
    return Error(PartLoc, "expected b, l, u, or t");

  StringRef Part = getLexer().getTok().getString();
  int64_t PartValue;
  if (Part == "b")
    PartValue = 0;
  else if (Part == "l")
    PartValue = 1;
  else if (Part == "u")
    PartValue = 2;
  else if (Part == "t")
    PartValue = 3;
  else
    return Error(PartLoc, "expected b, l, u, or t");

  const MCExpr *PartExpr = MCConstantExpr::create(PartValue, getContext());
  getLexer().Lex();
  Operands.push_back(
      AVR32Operand::createImm(PartExpr, PartLoc, getLexer().getLoc()));
  return false;
}

bool AVR32AsmParser::parseRegisterCommaRegisterHalfPartCommaRegisterHalfPart(
    OperandVector &Operands) {
  if (parseRegisterOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterHalfPart(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterHalfPart(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseLoadInsertOperands(OperandVector &Operands,
                                             bool IsByte) {
  if (parseRegisterOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Colon))
    return Error(getLexer().getLoc(), "expected :");
  Operands.push_back(AVR32Operand::createToken(":", getLexer().getLoc()));
  if (IsByte) {
    if (parseBytePartOperand(Operands))
      return true;
  } else if (parseHalfPartOperand(Operands)) {
    return true;
  }

  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseMemoryDispOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseLoadOperands(OperandVector &Operands,
                                       bool AllowBareRegister) {
  if (parseRegisterOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");

  if (parseOptionalToken(AsmToken::Minus)) {
    SMLoc MinusLoc = getLexer().getLoc();
    if (!parseOptionalToken(AsmToken::Minus))
      return Error(getLexer().getLoc(), "expected --");
    Operands.push_back(AVR32Operand::createToken("--", MinusLoc));
    if (parseRegisterOperand(Operands))
      return true;
    return false;
  }

  if (parseRegisterOperand(Operands))
    return true;

  if (AllowBareRegister && getLexer().is(AsmToken::EndOfStatement))
    return false;

  if (getLexer().is(AsmToken::LBrac)) {
    Operands.push_back(AVR32Operand::createToken("[", getLexer().getLoc()));
    getLexer().Lex();

    MCRegister IndexReg;
    SMLoc IndexStartLoc;
    SMLoc IndexEndLoc;
    ParseStatus IndexStatus =
        tryParseRegister(IndexReg, IndexStartLoc, IndexEndLoc);
    if (IndexStatus.isSuccess()) {
      Operands.push_back(
          AVR32Operand::createReg(IndexReg, IndexStartLoc, IndexEndLoc));
      if (parseOptionalToken(AsmToken::Colon)) {
        Operands.push_back(AVR32Operand::createToken(":", getLexer().getLoc()));
        if (parseBytePartOperand(Operands))
          return true;
      }
      if (getLexer().isNot(AsmToken::LessLess))
        return Error(getLexer().getLoc(), "expected <<");
      Operands.push_back(AVR32Operand::createToken("<<", getLexer().getLoc()));
      getLexer().Lex();
      if (parseImmediateOperand(Operands))
        return true;
    } else {
      if (IndexStatus.isFailure())
        return Error(IndexStartLoc, "invalid register name");
      if (parseImmediateOperand(Operands))
        return true;
    }

    if (getLexer().isNot(AsmToken::RBrac))
      return Error(getLexer().getLoc(), "expected ]");
    Operands.push_back(AVR32Operand::createToken("]", getLexer().getLoc()));
    getLexer().Lex();
    return false;
  }

  SMLoc PlusLoc = getLexer().getLoc();
  if (!parseOptionalToken(AsmToken::Plus) ||
      !parseOptionalToken(AsmToken::Plus))
    return Error(PlusLoc, "expected ++ or [disp]");
  Operands.push_back(AVR32Operand::createToken("++", PlusLoc));
  return false;
}

bool AVR32AsmParser::parseStoreHalfwordPairOperands(OperandVector &Operands) {
  if (parseRegisterOperand(Operands))
    return true;
  if (getLexer().isNot(AsmToken::LBrac))
    return Error(getLexer().getLoc(), "expected [");
  Operands.push_back(AVR32Operand::createToken("[", getLexer().getLoc()));
  getLexer().Lex();

  MCRegister IndexReg;
  SMLoc IndexStartLoc;
  SMLoc IndexEndLoc;
  ParseStatus IndexStatus =
      tryParseRegister(IndexReg, IndexStartLoc, IndexEndLoc);
  if (IndexStatus.isSuccess()) {
    Operands.push_back(
        AVR32Operand::createReg(IndexReg, IndexStartLoc, IndexEndLoc));
    if (getLexer().isNot(AsmToken::LessLess))
      return Error(getLexer().getLoc(), "expected <<");
    Operands.push_back(AVR32Operand::createToken("<<", getLexer().getLoc()));
    getLexer().Lex();
    if (parseImmediateOperand(Operands))
      return true;
  } else {
    if (IndexStatus.isFailure())
      return Error(IndexStartLoc, "invalid register name");
    if (parseImmediateOperand(Operands))
      return true;
  }

  if (getLexer().isNot(AsmToken::RBrac))
    return Error(getLexer().getLoc(), "expected ]");
  Operands.push_back(AVR32Operand::createToken("]", getLexer().getLoc()));
  getLexer().Lex();

  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterHalfPart(Operands))
    return true;

  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterHalfPart(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseStoreByteOperands(OperandVector &Operands) {
  if (parseOptionalToken(AsmToken::Minus)) {
    SMLoc MinusLoc = getLexer().getLoc();
    if (!parseOptionalToken(AsmToken::Minus))
      return Error(getLexer().getLoc(), "expected --");
    Operands.push_back(AVR32Operand::createToken("--", MinusLoc));
    if (parseRegisterOperand(Operands))
      return true;
  } else {
    if (parseRegisterOperand(Operands))
      return true;

    if (getLexer().is(AsmToken::LBrac)) {
      Operands.push_back(AVR32Operand::createToken("[", getLexer().getLoc()));
      getLexer().Lex();

      MCRegister IndexReg;
      SMLoc IndexStartLoc;
      SMLoc IndexEndLoc;
      ParseStatus IndexStatus =
          tryParseRegister(IndexReg, IndexStartLoc, IndexEndLoc);
      if (IndexStatus.isSuccess()) {
        Operands.push_back(
            AVR32Operand::createReg(IndexReg, IndexStartLoc, IndexEndLoc));
        if (getLexer().isNot(AsmToken::LessLess))
          return Error(getLexer().getLoc(), "expected <<");
        Operands.push_back(
            AVR32Operand::createToken("<<", getLexer().getLoc()));
        getLexer().Lex();
        if (parseImmediateOperand(Operands))
          return true;
      } else {
        if (IndexStatus.isFailure())
          return Error(IndexStartLoc, "invalid register name");
        if (parseImmediateOperand(Operands))
          return true;
      }

      if (getLexer().isNot(AsmToken::RBrac))
        return Error(getLexer().getLoc(), "expected ]");
      Operands.push_back(AVR32Operand::createToken("]", getLexer().getLoc()));
      getLexer().Lex();
    } else {
      SMLoc PlusLoc = getLexer().getLoc();
      if (!parseOptionalToken(AsmToken::Plus) ||
          !parseOptionalToken(AsmToken::Plus))
        return Error(PlusLoc, "expected ++ or [disp]");
      Operands.push_back(AVR32Operand::createToken("++", PlusLoc));
    }
  }

  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseStoreDoubleOperands(OperandVector &Operands) {
  if (parseOptionalToken(AsmToken::Minus)) {
    SMLoc MinusLoc = getLexer().getLoc();
    if (!parseOptionalToken(AsmToken::Minus))
      return Error(getLexer().getLoc(), "expected --");
    Operands.push_back(AVR32Operand::createToken("--", MinusLoc));
    if (parseRegisterOperand(Operands))
      return true;
  } else {
    if (parseRegisterOperand(Operands))
      return true;

    if (getLexer().is(AsmToken::LBrac)) {
      Operands.push_back(AVR32Operand::createToken("[", getLexer().getLoc()));
      getLexer().Lex();

      MCRegister IndexReg;
      SMLoc IndexStartLoc;
      SMLoc IndexEndLoc;
      ParseStatus IndexStatus =
          tryParseRegister(IndexReg, IndexStartLoc, IndexEndLoc);
      if (IndexStatus.isSuccess()) {
        Operands.push_back(
            AVR32Operand::createReg(IndexReg, IndexStartLoc, IndexEndLoc));
        if (getLexer().isNot(AsmToken::LessLess))
          return Error(getLexer().getLoc(), "expected <<");
        Operands.push_back(
            AVR32Operand::createToken("<<", getLexer().getLoc()));
        getLexer().Lex();
        if (parseImmediateOperand(Operands))
          return true;
      } else {
        if (IndexStatus.isFailure())
          return Error(IndexStartLoc, "invalid register name");
        if (parseImmediateOperand(Operands))
          return true;
      }

      if (getLexer().isNot(AsmToken::RBrac))
        return Error(getLexer().getLoc(), "expected ]");
      Operands.push_back(AVR32Operand::createToken("]", getLexer().getLoc()));
      getLexer().Lex();
    } else if (parseOptionalToken(AsmToken::Comma)) {
      if (parseRegisterOperand(Operands))
        return true;
      return false;
    } else {
      SMLoc PlusLoc = getLexer().getLoc();
      if (!parseOptionalToken(AsmToken::Plus) ||
          !parseOptionalToken(AsmToken::Plus))
        return Error(PlusLoc, "expected comma, ++, or [disp]");
      Operands.push_back(AVR32Operand::createToken("++", PlusLoc));
    }
  }

  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseMemoryDispOperand(OperandVector &Operands) {
  if (parseRegisterOperand(Operands))
    return true;

  if (getLexer().isNot(AsmToken::LBrac))
    return Error(getLexer().getLoc(), "expected [");
  Operands.push_back(AVR32Operand::createToken("[", getLexer().getLoc()));
  getLexer().Lex();

  if (parseImmediateOperand(Operands))
    return true;

  if (getLexer().isNot(AsmToken::RBrac))
    return Error(getLexer().getLoc(), "expected ]");
  Operands.push_back(AVR32Operand::createToken("]", getLexer().getLoc()));
  getLexer().Lex();
  return false;
}

bool AVR32AsmParser::parseMemoryDispCommaImmediate(OperandVector &Operands) {
  if (parseMemoryDispOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseImmediateOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseRegisterCommaImmediate(OperandVector &Operands) {
  if (parseRegisterOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseImmediateOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseRegisterCommaImmediateOptionalCOH(
    OperandVector &Operands) {
  if (parseRegisterCommaImmediate(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return false;

  if (getLexer().getKind() != AsmToken::Identifier ||
      !getParser().getTok().getString().equals_insensitive("coh"))
    return Error(getLexer().getLoc(), "expected coh");

  Operands.push_back(AVR32Operand::createToken("coh", getLexer().getLoc()));
  getLexer().Lex();
  return false;
}

bool AVR32AsmParser::parseRegisterCommaRegisterOrImmediate(
    OperandVector &Operands) {
  if (parseRegisterOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterOrImmediateOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseImmediateCommaRegister(OperandVector &Operands) {
  if (parseImmediateOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseRegisterOperand(Operands))
    return true;
  return false;
}

bool AVR32AsmParser::parseImmediateCommaImmediate(OperandVector &Operands) {
  if (parseImmediateOperand(Operands))
    return true;
  if (!parseOptionalToken(AsmToken::Comma))
    return Error(getLexer().getLoc(), "expected comma");
  if (parseImmediateOperand(Operands))
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
