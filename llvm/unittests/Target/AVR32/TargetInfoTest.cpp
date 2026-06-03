//===-- AVR32 TargetInfo tests -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/BinaryFormat/ELF.h"
#include "MCTargetDesc/AVR32MCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"
#include "gtest/gtest.h"
#include <optional>
#include <string>

using namespace llvm;

namespace {

TEST(AVR32TargetInfo, LookupTarget) {
  LLVMInitializeAVR32TargetInfo();
  LLVMInitializeAVR32TargetMC();
  LLVMInitializeAVR32AsmParser();

  Triple TT("avr32-unknown-unknown");
  std::string Error;
  const Target *TheTarget = TargetRegistry::lookupTarget(TT, Error);

  ASSERT_NE(TheTarget, nullptr) << Error;
  EXPECT_EQ(TheTarget->getName(), StringRef("avr32"));

  std::unique_ptr<TargetMachine> TM(TheTarget->createTargetMachine(
      TT, "", "", TargetOptions(), std::nullopt));
  EXPECT_EQ(TM.get(), nullptr);

  std::unique_ptr<MCRegisterInfo> MRI(TheTarget->createMCRegInfo(TT));
  ASSERT_NE(MRI.get(), nullptr);

  MCTargetOptions Options;
  std::unique_ptr<MCAsmInfo> MAI(
      TheTarget->createMCAsmInfo(*MRI, TT, Options));
  ASSERT_NE(MAI.get(), nullptr);
  EXPECT_EQ(MAI->getCodePointerSize(), 4u);
  EXPECT_EQ(MAI->getCommentString(), StringRef("#"));

  std::unique_ptr<MCInstrInfo> MII(TheTarget->createMCInstrInfo());
  ASSERT_NE(MII.get(), nullptr);
  EXPECT_EQ(MII->get(AVR32::NOP).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::MOVrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVri8).getSize(), 2u);
  EXPECT_EQ(MRI->getEncodingValue(AVR32::SP), 13u);
  EXPECT_EQ(MRI->getEncodingValue(AVR32::LR), 14u);
  EXPECT_EQ(MRI->getEncodingValue(AVR32::PC), 15u);

  std::unique_ptr<MCSubtargetInfo> STI(
      TheTarget->createMCSubtargetInfo(TT, "", ""));
  ASSERT_NE(STI.get(), nullptr);

  std::unique_ptr<MCAsmBackend> MAB(
      TheTarget->createMCAsmBackend(*STI, *MRI, Options));
  ASSERT_NE(MAB.get(), nullptr);
  EXPECT_EQ(MAB->Endian, llvm::endianness::big);

  std::unique_ptr<MCObjectTargetWriter> Writer =
      MAB->createObjectTargetWriter();
  ASSERT_NE(Writer.get(), nullptr);
  auto *ELFWriter = dyn_cast<MCELFObjectTargetWriter>(Writer.get());
  ASSERT_NE(ELFWriter, nullptr);
  EXPECT_EQ(ELFWriter->getEMachine(), ELF::EM_AVR32);
  EXPECT_FALSE(ELFWriter->is64Bit());

  MCContext Ctx(TT, *MAI, *MRI, *STI);
  std::unique_ptr<MCCodeEmitter> MCE(
      TheTarget->createMCCodeEmitter(*MII, Ctx));
  ASSERT_NE(MCE.get(), nullptr);

  MCInst Nop;
  Nop.setOpcode(AVR32::NOP);

  SmallVector<char, 2> Code;
  SmallVector<MCFixup, 0> Fixups;
  MCE->encodeInstruction(Nop, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd7);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);

  MCInst Mov;
  Mov.setOpcode(AVR32::MOVrr);
  Mov.addOperand(MCOperand::createReg(AVR32::R1));
  Mov.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Mov, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x17);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xf0);

  MCInst MovImm;
  MovImm.setOpcode(AVR32::MOVri8);
  MovImm.addOperand(MCOperand::createReg(AVR32::R1));
  MovImm.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(MovImm, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x3f);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf1);

  std::unique_ptr<MCInstPrinter> InstPrinter(
      TheTarget->createMCInstPrinter(TT, /*SyntaxVariant=*/0, *MAI, *MII,
                                     *MRI));
  ASSERT_NE(InstPrinter.get(), nullptr);

  std::string Printed;
  raw_string_ostream OS(Printed);
  InstPrinter->printInst(&Nop, /*Address=*/0, /*Annot=*/"", *STI, OS);
  OS.flush();
  EXPECT_EQ(Printed, "\tnop");

  Printed.clear();
  raw_string_ostream MovOS(Printed);
  InstPrinter->printInst(&Mov, /*Address=*/0, /*Annot=*/"", *STI, MovOS);
  MovOS.flush();
  EXPECT_EQ(Printed, "\tmov\tr1, r2");

  Printed.clear();
  raw_string_ostream MovImmOS(Printed);
  InstPrinter->printInst(&MovImm, /*Address=*/0, /*Annot=*/"", *STI, MovImmOS);
  MovImmOS.flush();
  EXPECT_EQ(Printed, "\tmov\tr1, -1");

  SourceMgr SrcMgr;
  SrcMgr.AddNewSourceBuffer(
      MemoryBuffer::getMemBuffer("nop\nmov r1, r2\nmov r1, -1\n"), SMLoc());

  MCContext ParseCtx(TT, *MAI, *MRI, *STI, &SrcMgr);
  std::unique_ptr<MCObjectFileInfo> MOFI(
      TheTarget->createMCObjectFileInfo(ParseCtx, /*PIC=*/false));
  ParseCtx.setObjectFileInfo(MOFI.get());

  std::unique_ptr<MCStreamer> Streamer(
      TheTarget->createNullStreamer(ParseCtx));
  std::unique_ptr<MCAsmParser> Parser(
      createMCAsmParser(SrcMgr, ParseCtx, *Streamer, *MAI));
  std::unique_ptr<MCTargetAsmParser> TargetParser(
      TheTarget->createMCAsmParser(*STI, *Parser, *MII));
  ASSERT_NE(TargetParser.get(), nullptr);
  Parser->setTargetParser(*TargetParser);
  EXPECT_FALSE(Parser->Run(/*NoInitialTextSection=*/false));
}

} // namespace
