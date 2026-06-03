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
  EXPECT_EQ(MII->get(AVR32::ABSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ACRr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ADDrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ANDNrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ANDrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::BREVr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CASTS_Br).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CASTS_Hr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CASTU_Br).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CASTU_Hr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::COMr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::EORrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::FRS).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::MOVrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVri8).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::NEGr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ORrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ROLr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RORr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RSUBrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SCRr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SUBrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SWAP_BHr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SWAP_Br).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SWAP_Hr).getSize(), 2u);
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

  MCInst Abs;
  Abs.setOpcode(AVR32::ABSr);
  Abs.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Abs, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x41);

  MCInst Acr;
  Acr.setOpcode(AVR32::ACRr);
  Acr.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Acr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);

  MCInst Add;
  Add.setOpcode(AVR32::ADDrr);
  Add.addOperand(MCOperand::createReg(AVR32::R1));
  Add.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Add, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);

  MCInst And;
  And.setOpcode(AVR32::ANDrr);
  And.addOperand(MCOperand::createReg(AVR32::R1));
  And.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(And, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x61);

  MCInst Andn;
  Andn.setOpcode(AVR32::ANDNrr);
  Andn.addOperand(MCOperand::createReg(AVR32::R1));
  Andn.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Andn, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x81);

  MCInst Brev;
  Brev.setOpcode(AVR32::BREVr);
  Brev.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Brev, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x91);

  MCInst CastsB;
  CastsB.setOpcode(AVR32::CASTS_Br);
  CastsB.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(CastsB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x61);

  MCInst CastsH;
  CastsH.setOpcode(AVR32::CASTS_Hr);
  CastsH.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(CastsH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x81);

  MCInst CastuB;
  CastuB.setOpcode(AVR32::CASTU_Br);
  CastuB.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(CastuB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x51);

  MCInst CastuH;
  CastuH.setOpcode(AVR32::CASTU_Hr);
  CastuH.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(CastuH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x71);

  MCInst Com;
  Com.setOpcode(AVR32::COMr);
  Com.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Com, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd1);

  MCInst Neg;
  Neg.setOpcode(AVR32::NEGr);
  Neg.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Neg, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x31);

  MCInst Eor;
  Eor.setOpcode(AVR32::EORrr);
  Eor.addOperand(MCOperand::createReg(AVR32::R1));
  Eor.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Eor, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x51);

  MCInst Frs;
  Frs.setOpcode(AVR32::FRS);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Frs, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd7);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x43);

  MCInst Or;
  Or.setOpcode(AVR32::ORrr);
  Or.addOperand(MCOperand::createReg(AVR32::R1));
  Or.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Or, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x41);

  MCInst Rol;
  Rol.setOpcode(AVR32::ROLr);
  Rol.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Rol, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf1);

  MCInst Ror;
  Ror.setOpcode(AVR32::RORr);
  Ror.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Ror, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5d);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);

  MCInst Rsub;
  Rsub.setOpcode(AVR32::RSUBrr);
  Rsub.addOperand(MCOperand::createReg(AVR32::R1));
  Rsub.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Rsub, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x21);

  MCInst Scr;
  Scr.setOpcode(AVR32::SCRr);
  Scr.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Scr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);

  MCInst Sub;
  Sub.setOpcode(AVR32::SUBrr);
  Sub.addOperand(MCOperand::createReg(AVR32::R1));
  Sub.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Sub, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);

  MCInst SwapBH;
  SwapBH.setOpcode(AVR32::SWAP_BHr);
  SwapBH.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SwapBH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xc1);

  MCInst SwapB;
  SwapB.setOpcode(AVR32::SWAP_Br);
  SwapB.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SwapB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);

  MCInst SwapH;
  SwapH.setOpcode(AVR32::SWAP_Hr);
  SwapH.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SwapH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xa1);

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
  raw_string_ostream AbsOS(Printed);
  InstPrinter->printInst(&Abs, /*Address=*/0, /*Annot=*/"", *STI, AbsOS);
  AbsOS.flush();
  EXPECT_EQ(Printed, "\tabs\tr1");

  Printed.clear();
  raw_string_ostream AcrOS(Printed);
  InstPrinter->printInst(&Acr, /*Address=*/0, /*Annot=*/"", *STI, AcrOS);
  AcrOS.flush();
  EXPECT_EQ(Printed, "\tacr\tr1");

  Printed.clear();
  raw_string_ostream AddOS(Printed);
  InstPrinter->printInst(&Add, /*Address=*/0, /*Annot=*/"", *STI, AddOS);
  AddOS.flush();
  EXPECT_EQ(Printed, "\tadd\tr1, r2");

  Printed.clear();
  raw_string_ostream AndOS(Printed);
  InstPrinter->printInst(&And, /*Address=*/0, /*Annot=*/"", *STI, AndOS);
  AndOS.flush();
  EXPECT_EQ(Printed, "\tand\tr1, r2");

  Printed.clear();
  raw_string_ostream AndnOS(Printed);
  InstPrinter->printInst(&Andn, /*Address=*/0, /*Annot=*/"", *STI, AndnOS);
  AndnOS.flush();
  EXPECT_EQ(Printed, "\tandn\tr1, r2");

  Printed.clear();
  raw_string_ostream BrevOS(Printed);
  InstPrinter->printInst(&Brev, /*Address=*/0, /*Annot=*/"", *STI, BrevOS);
  BrevOS.flush();
  EXPECT_EQ(Printed, "\tbrev\tr1");

  Printed.clear();
  raw_string_ostream CastsBOS(Printed);
  InstPrinter->printInst(&CastsB, /*Address=*/0, /*Annot=*/"", *STI, CastsBOS);
  CastsBOS.flush();
  EXPECT_EQ(Printed, "\tcasts.b\tr1");

  Printed.clear();
  raw_string_ostream CastsHOS(Printed);
  InstPrinter->printInst(&CastsH, /*Address=*/0, /*Annot=*/"", *STI, CastsHOS);
  CastsHOS.flush();
  EXPECT_EQ(Printed, "\tcasts.h\tr1");

  Printed.clear();
  raw_string_ostream CastuBOS(Printed);
  InstPrinter->printInst(&CastuB, /*Address=*/0, /*Annot=*/"", *STI, CastuBOS);
  CastuBOS.flush();
  EXPECT_EQ(Printed, "\tcastu.b\tr1");

  Printed.clear();
  raw_string_ostream CastuHOS(Printed);
  InstPrinter->printInst(&CastuH, /*Address=*/0, /*Annot=*/"", *STI, CastuHOS);
  CastuHOS.flush();
  EXPECT_EQ(Printed, "\tcastu.h\tr1");

  Printed.clear();
  raw_string_ostream ComOS(Printed);
  InstPrinter->printInst(&Com, /*Address=*/0, /*Annot=*/"", *STI, ComOS);
  ComOS.flush();
  EXPECT_EQ(Printed, "\tcom\tr1");

  Printed.clear();
  raw_string_ostream NegOS(Printed);
  InstPrinter->printInst(&Neg, /*Address=*/0, /*Annot=*/"", *STI, NegOS);
  NegOS.flush();
  EXPECT_EQ(Printed, "\tneg\tr1");

  Printed.clear();
  raw_string_ostream EorOS(Printed);
  InstPrinter->printInst(&Eor, /*Address=*/0, /*Annot=*/"", *STI, EorOS);
  EorOS.flush();
  EXPECT_EQ(Printed, "\teor\tr1, r2");

  Printed.clear();
  raw_string_ostream FrsOS(Printed);
  InstPrinter->printInst(&Frs, /*Address=*/0, /*Annot=*/"", *STI, FrsOS);
  FrsOS.flush();
  EXPECT_EQ(Printed, "\tfrs");

  Printed.clear();
  raw_string_ostream OrOS(Printed);
  InstPrinter->printInst(&Or, /*Address=*/0, /*Annot=*/"", *STI, OrOS);
  OrOS.flush();
  EXPECT_EQ(Printed, "\tor\tr1, r2");

  Printed.clear();
  raw_string_ostream RolOS(Printed);
  InstPrinter->printInst(&Rol, /*Address=*/0, /*Annot=*/"", *STI, RolOS);
  RolOS.flush();
  EXPECT_EQ(Printed, "\trol\tr1");

  Printed.clear();
  raw_string_ostream RorOS(Printed);
  InstPrinter->printInst(&Ror, /*Address=*/0, /*Annot=*/"", *STI, RorOS);
  RorOS.flush();
  EXPECT_EQ(Printed, "\tror\tr1");

  Printed.clear();
  raw_string_ostream RsubOS(Printed);
  InstPrinter->printInst(&Rsub, /*Address=*/0, /*Annot=*/"", *STI, RsubOS);
  RsubOS.flush();
  EXPECT_EQ(Printed, "\trsub\tr1, r2");

  Printed.clear();
  raw_string_ostream ScrOS(Printed);
  InstPrinter->printInst(&Scr, /*Address=*/0, /*Annot=*/"", *STI, ScrOS);
  ScrOS.flush();
  EXPECT_EQ(Printed, "\tscr\tr1");

  Printed.clear();
  raw_string_ostream SubOS(Printed);
  InstPrinter->printInst(&Sub, /*Address=*/0, /*Annot=*/"", *STI, SubOS);
  SubOS.flush();
  EXPECT_EQ(Printed, "\tsub\tr1, r2");

  Printed.clear();
  raw_string_ostream SwapBHOS(Printed);
  InstPrinter->printInst(&SwapBH, /*Address=*/0, /*Annot=*/"", *STI, SwapBHOS);
  SwapBHOS.flush();
  EXPECT_EQ(Printed, "\tswap.bh\tr1");

  Printed.clear();
  raw_string_ostream SwapBOS(Printed);
  InstPrinter->printInst(&SwapB, /*Address=*/0, /*Annot=*/"", *STI, SwapBOS);
  SwapBOS.flush();
  EXPECT_EQ(Printed, "\tswap.b\tr1");

  Printed.clear();
  raw_string_ostream SwapHOS(Printed);
  InstPrinter->printInst(&SwapH, /*Address=*/0, /*Annot=*/"", *STI, SwapHOS);
  SwapHOS.flush();
  EXPECT_EQ(Printed, "\tswap.h\tr1");

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
      MemoryBuffer::getMemBuffer(
          "nop\nfrs\nabs r1\nacr r1\nadd r1, r2\nand r1, r2\nandn r1, r2\nbrev r1\ncasts.b r1\ncasts.h r1\ncastu.b r1\ncastu.h r1\ncom r1\nneg r1\neor r1, r2\nor r1, r2\nrol r1\nror r1\nrsub r1, r2\nscr r1\nsub r1, r2\nswap.bh r1\nswap.b r1\nswap.h r1\nmov r1, r2\nmov r1, -1\n"),
      SMLoc());

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
