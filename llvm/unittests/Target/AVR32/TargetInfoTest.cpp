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
  EXPECT_EQ(MII->get(AVR32::ACALLi).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ADCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ABSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ACRr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ADDABSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ANDNrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ANDrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ASRrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::BLDri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::BREVr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::BSTri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::BREAKPOINT).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CBRri).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CASTS_Br).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CASTS_Hr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CASTU_Br).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CASTU_Hr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CLZrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::COMr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CPCr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CPCrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::CP_Wri21).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::CP_Wri6).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CP_Wrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CSRFCZi).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CSRFi).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::EORrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::FRS).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ICALLr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::INCJOSPi).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LSLrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LSRrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MAXrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MINrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MFDRi).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MFSRi).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVri8).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::MOVHri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MULrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::MULri8).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MULrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MULS_Drrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MULU_Drrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MUSFRr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::MUSTRr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::MTDRi).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MTSRi).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::NEGr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ORrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::POPJC).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::PUSHJC).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETD).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETE).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETALr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETCCr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETCSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETEQr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETGEr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETGTr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETHIr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETHSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETLEr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETLOr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETLSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETLTr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETMIr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETNEr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETPLr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETQSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETVCr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETVSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETJ).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETS).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RETSS).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ROLr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RORr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::RSUBrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SBCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SBRri).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SCALL).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SSCALL).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SCRr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SLEEPi).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SRALr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRCCr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRCSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SREQr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRGEr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRGTr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRHIr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRHSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRLEr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRLOr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRLSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRLTr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRMIr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRNEr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRPLr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRQSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRVCr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SRVSr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SSRFi).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SUBrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SWAP_BHr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SWAP_Br).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SWAP_Hr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SYNCi).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::TLBR).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::TLBS).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::TLBW).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::TNBZr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::TSTrr).getSize(), 2u);

  auto ExpectFixedReturn = [&](unsigned Opcode) {
    const MCInstrDesc &Desc = MII->get(Opcode);
    EXPECT_TRUE(Desc.isReturn());
    EXPECT_TRUE(Desc.isTerminator());
    EXPECT_TRUE(Desc.isBarrier());
    EXPECT_FALSE(Desc.isBranch());
  };

  for (unsigned Opcode :
       {AVR32::RETD, AVR32::RETE, AVR32::RETJ, AVR32::RETS, AVR32::RETSS})
    ExpectFixedReturn(Opcode);

  auto ExpectConditionalReturn = [&](unsigned Opcode) {
    const MCInstrDesc &Desc = MII->get(Opcode);
    EXPECT_TRUE(Desc.isReturn());
    EXPECT_TRUE(Desc.isTerminator());
    EXPECT_TRUE(Desc.isBranch());
    EXPECT_FALSE(Desc.isBarrier());
  };

  for (unsigned Opcode :
       {AVR32::RETALr, AVR32::RETCCr, AVR32::RETCSr, AVR32::RETEQr,
        AVR32::RETGEr, AVR32::RETGTr, AVR32::RETHIr, AVR32::RETHSr,
        AVR32::RETLEr, AVR32::RETLOr, AVR32::RETLSr, AVR32::RETLTr,
        AVR32::RETMIr, AVR32::RETNEr, AVR32::RETPLr, AVR32::RETQSr,
        AVR32::RETVCr, AVR32::RETVSr})
    ExpectConditionalReturn(Opcode);

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

  MCInst ACall;
  ACall.setOpcode(AVR32::ACALLi);
  ACall.addOperand(MCOperand::createImm(4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(ACall, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd0);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x10);

  MCInst Adc;
  Adc.setOpcode(AVR32::ADCrrr);
  Adc.addOperand(MCOperand::createReg(AVR32::R1));
  Adc.addOperand(MCOperand::createReg(AVR32::R2));
  Adc.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Adc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst AddAbs;
  AddAbs.setOpcode(AVR32::ADDABSrrr);
  AddAbs.addOperand(MCOperand::createReg(AVR32::R1));
  AddAbs.addOperand(MCOperand::createReg(AVR32::R2));
  AddAbs.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(AddAbs, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0e);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

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

  MCInst Asr;
  Asr.setOpcode(AVR32::ASRrrr);
  Asr.addOperand(MCOperand::createReg(AVR32::R1));
  Asr.addOperand(MCOperand::createReg(AVR32::R2));
  Asr.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Asr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x08);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst Bld;
  Bld.setOpcode(AVR32::BLDri);
  Bld.addOperand(MCOperand::createReg(AVR32::R1));
  Bld.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Bld, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xed);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

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

  MCInst Bst;
  Bst.setOpcode(AVR32::BSTri);
  Bst.addOperand(MCOperand::createReg(AVR32::R1));
  Bst.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Bst, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xef);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst Cbr;
  Cbr.setOpcode(AVR32::CBRri);
  Cbr.addOperand(MCOperand::createReg(AVR32::R1));
  Cbr.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Cbr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xa1);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd1);

  MCInst Breakpoint;
  Breakpoint.setOpcode(AVR32::BREAKPOINT);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Breakpoint, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x73);

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

  MCInst Clz;
  Clz.setOpcode(AVR32::CLZrr);
  Clz.addOperand(MCOperand::createReg(AVR32::R1));
  Clz.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Clz, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x12);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x00);

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

  MCInst Cpc;
  Cpc.setOpcode(AVR32::CPCr);
  Cpc.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Cpc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x21);

  MCInst CpcRR;
  CpcRR.setOpcode(AVR32::CPCrr);
  CpcRR.addOperand(MCOperand::createReg(AVR32::R1));
  CpcRR.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(CpcRR, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x13);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x00);

  MCInst CpW;
  CpW.setOpcode(AVR32::CP_Wrr);
  CpW.addOperand(MCOperand::createReg(AVR32::R1));
  CpW.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(CpW, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x31);

  MCInst CpWImm;
  CpWImm.setOpcode(AVR32::CP_Wri6);
  CpWImm.addOperand(MCOperand::createReg(AVR32::R1));
  CpWImm.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(CpWImm, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5b);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf1);

  MCInst CpWImm21;
  CpWImm21.setOpcode(AVR32::CP_Wri21);
  CpWImm21.addOperand(MCOperand::createReg(AVR32::R1));
  CpWImm21.addOperand(MCOperand::createImm(32));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(CpWImm21, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe0);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x41);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x20);

  MCInst Csrfcz;
  Csrfcz.setOpcode(AVR32::CSRFCZi);
  Csrfcz.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Csrfcz, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd0);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x13);

  MCInst Csrf;
  Csrf.setOpcode(AVR32::CSRFi);
  Csrf.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Csrf, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x13);

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

  MCInst Incjosp;
  Incjosp.setOpcode(AVR32::INCJOSPi);
  Incjosp.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Incjosp, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf3);

  MCInst Icall;
  Icall.setOpcode(AVR32::ICALLr);
  Icall.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Icall, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5d);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);

  MCInst Lsl;
  Lsl.setOpcode(AVR32::LSLrrr);
  Lsl.addOperand(MCOperand::createReg(AVR32::R1));
  Lsl.addOperand(MCOperand::createReg(AVR32::R2));
  Lsl.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Lsl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x09);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst Lsr;
  Lsr.setOpcode(AVR32::LSRrrr);
  Lsr.addOperand(MCOperand::createReg(AVR32::R1));
  Lsr.addOperand(MCOperand::createReg(AVR32::R2));
  Lsr.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Lsr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0a);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst Max;
  Max.setOpcode(AVR32::MAXrrr);
  Max.addOperand(MCOperand::createReg(AVR32::R1));
  Max.addOperand(MCOperand::createReg(AVR32::R2));
  Max.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Max, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0c);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst Min;
  Min.setOpcode(AVR32::MINrrr);
  Min.addOperand(MCOperand::createReg(AVR32::R1));
  Min.addOperand(MCOperand::createReg(AVR32::R2));
  Min.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Min, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0d);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst Mfdr;
  Mfdr.setOpcode(AVR32::MFDRi);
  Mfdr.addOperand(MCOperand::createReg(AVR32::R1));
  Mfdr.addOperand(MCOperand::createImm(4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Mfdr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst Mfsr;
  Mfsr.setOpcode(AVR32::MFSRi);
  Mfsr.addOperand(MCOperand::createReg(AVR32::R1));
  Mfsr.addOperand(MCOperand::createImm(4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Mfsr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe1);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst MulCompact;
  MulCompact.setOpcode(AVR32::MULrr);
  MulCompact.addOperand(MCOperand::createReg(AVR32::R1));
  MulCompact.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(MulCompact, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xa5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x31);

  MCInst MulImm;
  MulImm.setOpcode(AVR32::MULri8);
  MulImm.addOperand(MCOperand::createReg(AVR32::R1));
  MulImm.addOperand(MCOperand::createReg(AVR32::R2));
  MulImm.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(MulImm, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x10);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst Mul;
  Mul.setOpcode(AVR32::MULrrr);
  Mul.addOperand(MCOperand::createReg(AVR32::R1));
  Mul.addOperand(MCOperand::createReg(AVR32::R2));
  Mul.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Mul, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst MulsD;
  MulsD.setOpcode(AVR32::MULS_Drrr);
  MulsD.addOperand(MCOperand::createReg(AVR32::R2));
  MulsD.addOperand(MCOperand::createReg(AVR32::R3));
  MulsD.addOperand(MCOperand::createReg(AVR32::R4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(MulsD, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x42);

  MCInst MuluD;
  MuluD.setOpcode(AVR32::MULU_Drrr);
  MuluD.addOperand(MCOperand::createReg(AVR32::R2));
  MuluD.addOperand(MCOperand::createReg(AVR32::R3));
  MuluD.addOperand(MCOperand::createReg(AVR32::R4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(MuluD, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x06);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x42);

  MCInst Musfr;
  Musfr.setOpcode(AVR32::MUSFRr);
  Musfr.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Musfr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5d);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x31);

  MCInst Mustr;
  Mustr.setOpcode(AVR32::MUSTRr);
  Mustr.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Mustr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5d);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x21);

  MCInst Mtdr;
  Mtdr.setOpcode(AVR32::MTDRi);
  Mtdr.addOperand(MCOperand::createImm(4));
  Mtdr.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Mtdr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe7);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst Mtsr;
  Mtsr.setOpcode(AVR32::MTSRi);
  Mtsr.addOperand(MCOperand::createImm(4));
  Mtsr.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Mtsr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

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

  MCInst Popjc;
  Popjc.setOpcode(AVR32::POPJC);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Popjc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd7);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x13);

  MCInst Pushjc;
  Pushjc.setOpcode(AVR32::PUSHJC);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Pushjc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd7);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x23);

  MCInst Retd;
  Retd.setOpcode(AVR32::RETD);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retd, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x23);

  MCInst Rete;
  Rete.setOpcode(AVR32::RETE);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Rete, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);

  MCInst Retal;
  Retal.setOpcode(AVR32::RETALr);
  Retal.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retal, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xfe);

  MCInst Retcc;
  Retcc.setOpcode(AVR32::RETCCr);
  Retcc.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retcc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x2e);

  MCInst Retcs;
  Retcs.setOpcode(AVR32::RETCSr);
  Retcs.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retcs, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x3e);

  MCInst Retlo;
  Retlo.setOpcode(AVR32::RETLOr);
  Retlo.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retlo, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x3e);

  MCInst Retge;
  Retge.setOpcode(AVR32::RETGEr);
  Retge.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retge, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x4e);

  MCInst Retlt;
  Retlt.setOpcode(AVR32::RETLTr);
  Retlt.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retlt, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x5e);

  MCInst Retmi;
  Retmi.setOpcode(AVR32::RETMIr);
  Retmi.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retmi, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x6e);

  MCInst Retpl;
  Retpl.setOpcode(AVR32::RETPLr);
  Retpl.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retpl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x7e);

  MCInst Retls;
  Retls.setOpcode(AVR32::RETLSr);
  Retls.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retls, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x8e);

  MCInst Retgt;
  Retgt.setOpcode(AVR32::RETGTr);
  Retgt.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retgt, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x9e);

  MCInst Retle;
  Retle.setOpcode(AVR32::RETLEr);
  Retle.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retle, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xae);

  MCInst Rethi;
  Rethi.setOpcode(AVR32::RETHIr);
  Rethi.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Rethi, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xbe);

  MCInst Retvs;
  Retvs.setOpcode(AVR32::RETVSr);
  Retvs.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retvs, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xce);

  MCInst Retvc;
  Retvc.setOpcode(AVR32::RETVCr);
  Retvc.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retvc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xde);

  MCInst Retqs;
  Retqs.setOpcode(AVR32::RETQSr);
  Retqs.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retqs, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xee);

  MCInst Reths;
  Reths.setOpcode(AVR32::RETHSr);
  Reths.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Reths, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x2e);

  MCInst Reteq;
  Reteq.setOpcode(AVR32::RETEQr);
  Reteq.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Reteq, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x0e);

  MCInst Retne;
  Retne.setOpcode(AVR32::RETNEr);
  Retne.addOperand(MCOperand::createReg(AVR32::LR));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retne, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5e);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x1e);

  MCInst Retj;
  Retj.setOpcode(AVR32::RETJ);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retj, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x33);

  MCInst Rets;
  Rets.setOpcode(AVR32::RETS);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Rets, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x13);

  MCInst Retss;
  Retss.setOpcode(AVR32::RETSS);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Retss, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd7);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x63);

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

  MCInst Sbc;
  Sbc.setOpcode(AVR32::SBCrrr);
  Sbc.addOperand(MCOperand::createReg(AVR32::R1));
  Sbc.addOperand(MCOperand::createReg(AVR32::R2));
  Sbc.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Sbc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst Sbr;
  Sbr.setOpcode(AVR32::SBRri);
  Sbr.addOperand(MCOperand::createReg(AVR32::R1));
  Sbr.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Sbr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xa1);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);

  MCInst Scall;
  Scall.setOpcode(AVR32::SCALL);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Scall, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd7);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x33);

  MCInst Sscall;
  Sscall.setOpcode(AVR32::SSCALL);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Sscall, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd7);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x53);

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

  MCInst SrEq;
  SrEq.setOpcode(AVR32::SREQr);
  SrEq.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SrEq, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5f);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);

  MCInst SrAl;
  SrAl.setOpcode(AVR32::SRALr);
  SrAl.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SrAl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5f);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf1);

  MCInst Sleep;
  Sleep.setOpcode(AVR32::SLEEPi);
  Sleep.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Sleep, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x60);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst Ssrf;
  Ssrf.setOpcode(AVR32::SSRFi);
  Ssrf.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Ssrf, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x13);

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

  MCInst Sync;
  Sync.setOpcode(AVR32::SYNCi);
  Sync.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Sync, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe7);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x60);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst Tlbr;
  Tlbr.setOpcode(AVR32::TLBR);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Tlbr, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x43);

  MCInst Tlbs;
  Tlbs.setOpcode(AVR32::TLBS);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Tlbs, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x53);

  MCInst Tlbw;
  Tlbw.setOpcode(AVR32::TLBW);

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Tlbw, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xd6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x63);

  MCInst Tnbz;
  Tnbz.setOpcode(AVR32::TNBZr);
  Tnbz.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Tnbz, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x5c);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xe1);

  MCInst Tst;
  Tst.setOpcode(AVR32::TSTrr);
  Tst.addOperand(MCOperand::createReg(AVR32::R1));
  Tst.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Tst, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x71);

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

  MCInst Movh;
  Movh.setOpcode(AVR32::MOVHri);
  Movh.addOperand(MCOperand::createReg(AVR32::R1));
  Movh.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Movh, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xfc);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

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
  raw_string_ostream ACallOS(Printed);
  InstPrinter->printInst(&ACall, /*Address=*/0, /*Annot=*/"", *STI, ACallOS);
  ACallOS.flush();
  EXPECT_EQ(Printed, "\tacall\t4");

  Printed.clear();
  raw_string_ostream AdcOS(Printed);
  InstPrinter->printInst(&Adc, /*Address=*/0, /*Annot=*/"", *STI, AdcOS);
  AdcOS.flush();
  EXPECT_EQ(Printed, "\tadc\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream AddAbsOS(Printed);
  InstPrinter->printInst(&AddAbs, /*Address=*/0, /*Annot=*/"", *STI,
                         AddAbsOS);
  AddAbsOS.flush();
  EXPECT_EQ(Printed, "\taddabs\tr1, r2, r3");

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
  raw_string_ostream AsrOS(Printed);
  InstPrinter->printInst(&Asr, /*Address=*/0, /*Annot=*/"", *STI, AsrOS);
  AsrOS.flush();
  EXPECT_EQ(Printed, "\tasr\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream BldOS(Printed);
  InstPrinter->printInst(&Bld, /*Address=*/0, /*Annot=*/"", *STI, BldOS);
  BldOS.flush();
  EXPECT_EQ(Printed, "\tbld\tr1, 1");

  Printed.clear();
  raw_string_ostream BrevOS(Printed);
  InstPrinter->printInst(&Brev, /*Address=*/0, /*Annot=*/"", *STI, BrevOS);
  BrevOS.flush();
  EXPECT_EQ(Printed, "\tbrev\tr1");

  Printed.clear();
  raw_string_ostream BstOS(Printed);
  InstPrinter->printInst(&Bst, /*Address=*/0, /*Annot=*/"", *STI, BstOS);
  BstOS.flush();
  EXPECT_EQ(Printed, "\tbst\tr1, 1");

  Printed.clear();
  raw_string_ostream CbrOS(Printed);
  InstPrinter->printInst(&Cbr, /*Address=*/0, /*Annot=*/"", *STI, CbrOS);
  CbrOS.flush();
  EXPECT_EQ(Printed, "\tcbr\tr1, 1");

  Printed.clear();
  raw_string_ostream BreakpointOS(Printed);
  InstPrinter->printInst(&Breakpoint, /*Address=*/0, /*Annot=*/"", *STI,
                         BreakpointOS);
  BreakpointOS.flush();
  EXPECT_EQ(Printed, "\tbreakpoint");

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
  raw_string_ostream CpcOS(Printed);
  InstPrinter->printInst(&Cpc, /*Address=*/0, /*Annot=*/"", *STI, CpcOS);
  CpcOS.flush();
  EXPECT_EQ(Printed, "\tcpc\tr1");

  Printed.clear();
  raw_string_ostream CpcRROS(Printed);
  InstPrinter->printInst(&CpcRR, /*Address=*/0, /*Annot=*/"", *STI,
                         CpcRROS);
  CpcRROS.flush();
  EXPECT_EQ(Printed, "\tcpc\tr1, r2");

  Printed.clear();
  raw_string_ostream ClzOS(Printed);
  InstPrinter->printInst(&Clz, /*Address=*/0, /*Annot=*/"", *STI, ClzOS);
  ClzOS.flush();
  EXPECT_EQ(Printed, "\tclz\tr1, r2");

  Printed.clear();
  raw_string_ostream CpWOS(Printed);
  InstPrinter->printInst(&CpW, /*Address=*/0, /*Annot=*/"", *STI, CpWOS);
  CpWOS.flush();
  EXPECT_EQ(Printed, "\tcp.w\tr1, r2");

  Printed.clear();
  raw_string_ostream CpWImmOS(Printed);
  InstPrinter->printInst(&CpWImm, /*Address=*/0, /*Annot=*/"", *STI,
                         CpWImmOS);
  CpWImmOS.flush();
  EXPECT_EQ(Printed, "\tcp.w\tr1, -1");

  Printed.clear();
  raw_string_ostream CpWImm21OS(Printed);
  InstPrinter->printInst(&CpWImm21, /*Address=*/0, /*Annot=*/"", *STI,
                         CpWImm21OS);
  CpWImm21OS.flush();
  EXPECT_EQ(Printed, "\tcp.w\tr1, 32");

  Printed.clear();
  raw_string_ostream CsrfczOS(Printed);
  InstPrinter->printInst(&Csrfcz, /*Address=*/0, /*Annot=*/"", *STI,
                         CsrfczOS);
  CsrfczOS.flush();
  EXPECT_EQ(Printed, "\tcsrfcz\t1");

  Printed.clear();
  raw_string_ostream CsrfOS(Printed);
  InstPrinter->printInst(&Csrf, /*Address=*/0, /*Annot=*/"", *STI, CsrfOS);
  CsrfOS.flush();
  EXPECT_EQ(Printed, "\tcsrf\t1");

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
  raw_string_ostream IncjospOS(Printed);
  InstPrinter->printInst(&Incjosp, /*Address=*/0, /*Annot=*/"", *STI,
                         IncjospOS);
  IncjospOS.flush();
  EXPECT_EQ(Printed, "\tincjosp\t-1");

  Printed.clear();
  raw_string_ostream IcallOS(Printed);
  InstPrinter->printInst(&Icall, /*Address=*/0, /*Annot=*/"", *STI, IcallOS);
  IcallOS.flush();
  EXPECT_EQ(Printed, "\ticall\tr1");

  Printed.clear();
  raw_string_ostream LslOS(Printed);
  InstPrinter->printInst(&Lsl, /*Address=*/0, /*Annot=*/"", *STI, LslOS);
  LslOS.flush();
  EXPECT_EQ(Printed, "\tlsl\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream LsrOS(Printed);
  InstPrinter->printInst(&Lsr, /*Address=*/0, /*Annot=*/"", *STI, LsrOS);
  LsrOS.flush();
  EXPECT_EQ(Printed, "\tlsr\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream MaxOS(Printed);
  InstPrinter->printInst(&Max, /*Address=*/0, /*Annot=*/"", *STI, MaxOS);
  MaxOS.flush();
  EXPECT_EQ(Printed, "\tmax\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream MinOS(Printed);
  InstPrinter->printInst(&Min, /*Address=*/0, /*Annot=*/"", *STI, MinOS);
  MinOS.flush();
  EXPECT_EQ(Printed, "\tmin\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream MfdrOS(Printed);
  InstPrinter->printInst(&Mfdr, /*Address=*/0, /*Annot=*/"", *STI, MfdrOS);
  MfdrOS.flush();
  EXPECT_EQ(Printed, "\tmfdr\tr1, 4");

  Printed.clear();
  raw_string_ostream MfsrOS(Printed);
  InstPrinter->printInst(&Mfsr, /*Address=*/0, /*Annot=*/"", *STI, MfsrOS);
  MfsrOS.flush();
  EXPECT_EQ(Printed, "\tmfsr\tr1, 4");

  Printed.clear();
  raw_string_ostream MulCompactOS(Printed);
  InstPrinter->printInst(&MulCompact, /*Address=*/0, /*Annot=*/"", *STI,
                         MulCompactOS);
  MulCompactOS.flush();
  EXPECT_EQ(Printed, "\tmul\tr1, r2");

  Printed.clear();
  raw_string_ostream MulImmOS(Printed);
  InstPrinter->printInst(&MulImm, /*Address=*/0, /*Annot=*/"", *STI,
                         MulImmOS);
  MulImmOS.flush();
  EXPECT_EQ(Printed, "\tmul\tr1, r2, -1");

  Printed.clear();
  raw_string_ostream MulOS(Printed);
  InstPrinter->printInst(&Mul, /*Address=*/0, /*Annot=*/"", *STI, MulOS);
  MulOS.flush();
  EXPECT_EQ(Printed, "\tmul\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream MulsDOS(Printed);
  InstPrinter->printInst(&MulsD, /*Address=*/0, /*Annot=*/"", *STI, MulsDOS);
  MulsDOS.flush();
  EXPECT_EQ(Printed, "\tmuls.d\tr2, r3, r4");

  Printed.clear();
  raw_string_ostream MuluDOS(Printed);
  InstPrinter->printInst(&MuluD, /*Address=*/0, /*Annot=*/"", *STI, MuluDOS);
  MuluDOS.flush();
  EXPECT_EQ(Printed, "\tmulu.d\tr2, r3, r4");

  Printed.clear();
  raw_string_ostream MusfrOS(Printed);
  InstPrinter->printInst(&Musfr, /*Address=*/0, /*Annot=*/"", *STI, MusfrOS);
  MusfrOS.flush();
  EXPECT_EQ(Printed, "\tmusfr\tr1");

  Printed.clear();
  raw_string_ostream MustrOS(Printed);
  InstPrinter->printInst(&Mustr, /*Address=*/0, /*Annot=*/"", *STI, MustrOS);
  MustrOS.flush();
  EXPECT_EQ(Printed, "\tmustr\tr1");

  Printed.clear();
  raw_string_ostream MtdrOS(Printed);
  InstPrinter->printInst(&Mtdr, /*Address=*/0, /*Annot=*/"", *STI, MtdrOS);
  MtdrOS.flush();
  EXPECT_EQ(Printed, "\tmtdr\t4, r1");

  Printed.clear();
  raw_string_ostream MtsrOS(Printed);
  InstPrinter->printInst(&Mtsr, /*Address=*/0, /*Annot=*/"", *STI, MtsrOS);
  MtsrOS.flush();
  EXPECT_EQ(Printed, "\tmtsr\t4, r1");

  Printed.clear();
  raw_string_ostream OrOS(Printed);
  InstPrinter->printInst(&Or, /*Address=*/0, /*Annot=*/"", *STI, OrOS);
  OrOS.flush();
  EXPECT_EQ(Printed, "\tor\tr1, r2");

  Printed.clear();
  raw_string_ostream PopjcOS(Printed);
  InstPrinter->printInst(&Popjc, /*Address=*/0, /*Annot=*/"", *STI, PopjcOS);
  PopjcOS.flush();
  EXPECT_EQ(Printed, "\tpopjc");

  Printed.clear();
  raw_string_ostream PushjcOS(Printed);
  InstPrinter->printInst(&Pushjc, /*Address=*/0, /*Annot=*/"", *STI,
                         PushjcOS);
  PushjcOS.flush();
  EXPECT_EQ(Printed, "\tpushjc");

  Printed.clear();
  raw_string_ostream RetdOS(Printed);
  InstPrinter->printInst(&Retd, /*Address=*/0, /*Annot=*/"", *STI, RetdOS);
  RetdOS.flush();
  EXPECT_EQ(Printed, "\tretd");

  Printed.clear();
  raw_string_ostream ReteOS(Printed);
  InstPrinter->printInst(&Rete, /*Address=*/0, /*Annot=*/"", *STI, ReteOS);
  ReteOS.flush();
  EXPECT_EQ(Printed, "\trete");

  Printed.clear();
  raw_string_ostream RetalOS(Printed);
  InstPrinter->printInst(&Retal, /*Address=*/0, /*Annot=*/"", *STI, RetalOS);
  RetalOS.flush();
  EXPECT_EQ(Printed, "\tretal\tlr");

  Printed.clear();
  raw_string_ostream RetccOS(Printed);
  InstPrinter->printInst(&Retcc, /*Address=*/0, /*Annot=*/"", *STI, RetccOS);
  RetccOS.flush();
  EXPECT_EQ(Printed, "\tretcc\tlr");

  Printed.clear();
  raw_string_ostream RetcsOS(Printed);
  InstPrinter->printInst(&Retcs, /*Address=*/0, /*Annot=*/"", *STI, RetcsOS);
  RetcsOS.flush();
  EXPECT_EQ(Printed, "\tretcs\tlr");

  Printed.clear();
  raw_string_ostream RetloOS(Printed);
  InstPrinter->printInst(&Retlo, /*Address=*/0, /*Annot=*/"", *STI, RetloOS);
  RetloOS.flush();
  EXPECT_EQ(Printed, "\tretlo\tlr");

  Printed.clear();
  raw_string_ostream RetgeOS(Printed);
  InstPrinter->printInst(&Retge, /*Address=*/0, /*Annot=*/"", *STI, RetgeOS);
  RetgeOS.flush();
  EXPECT_EQ(Printed, "\tretge\tlr");

  Printed.clear();
  raw_string_ostream RetltOS(Printed);
  InstPrinter->printInst(&Retlt, /*Address=*/0, /*Annot=*/"", *STI, RetltOS);
  RetltOS.flush();
  EXPECT_EQ(Printed, "\tretlt\tlr");

  Printed.clear();
  raw_string_ostream RetmiOS(Printed);
  InstPrinter->printInst(&Retmi, /*Address=*/0, /*Annot=*/"", *STI, RetmiOS);
  RetmiOS.flush();
  EXPECT_EQ(Printed, "\tretmi\tlr");

  Printed.clear();
  raw_string_ostream RetplOS(Printed);
  InstPrinter->printInst(&Retpl, /*Address=*/0, /*Annot=*/"", *STI, RetplOS);
  RetplOS.flush();
  EXPECT_EQ(Printed, "\tretpl\tlr");

  Printed.clear();
  raw_string_ostream RetlsOS(Printed);
  InstPrinter->printInst(&Retls, /*Address=*/0, /*Annot=*/"", *STI, RetlsOS);
  RetlsOS.flush();
  EXPECT_EQ(Printed, "\tretls\tlr");

  Printed.clear();
  raw_string_ostream RetgtOS(Printed);
  InstPrinter->printInst(&Retgt, /*Address=*/0, /*Annot=*/"", *STI, RetgtOS);
  RetgtOS.flush();
  EXPECT_EQ(Printed, "\tretgt\tlr");

  Printed.clear();
  raw_string_ostream RetleOS(Printed);
  InstPrinter->printInst(&Retle, /*Address=*/0, /*Annot=*/"", *STI, RetleOS);
  RetleOS.flush();
  EXPECT_EQ(Printed, "\tretle\tlr");

  Printed.clear();
  raw_string_ostream RethiOS(Printed);
  InstPrinter->printInst(&Rethi, /*Address=*/0, /*Annot=*/"", *STI, RethiOS);
  RethiOS.flush();
  EXPECT_EQ(Printed, "\trethi\tlr");

  Printed.clear();
  raw_string_ostream RetvsOS(Printed);
  InstPrinter->printInst(&Retvs, /*Address=*/0, /*Annot=*/"", *STI, RetvsOS);
  RetvsOS.flush();
  EXPECT_EQ(Printed, "\tretvs\tlr");

  Printed.clear();
  raw_string_ostream RetvcOS(Printed);
  InstPrinter->printInst(&Retvc, /*Address=*/0, /*Annot=*/"", *STI, RetvcOS);
  RetvcOS.flush();
  EXPECT_EQ(Printed, "\tretvc\tlr");

  Printed.clear();
  raw_string_ostream RetqsOS(Printed);
  InstPrinter->printInst(&Retqs, /*Address=*/0, /*Annot=*/"", *STI, RetqsOS);
  RetqsOS.flush();
  EXPECT_EQ(Printed, "\tretqs\tlr");

  Printed.clear();
  raw_string_ostream RethsOS(Printed);
  InstPrinter->printInst(&Reths, /*Address=*/0, /*Annot=*/"", *STI, RethsOS);
  RethsOS.flush();
  EXPECT_EQ(Printed, "\treths\tlr");

  Printed.clear();
  raw_string_ostream ReteqOS(Printed);
  InstPrinter->printInst(&Reteq, /*Address=*/0, /*Annot=*/"", *STI, ReteqOS);
  ReteqOS.flush();
  EXPECT_EQ(Printed, "\treteq\tlr");

  Printed.clear();
  raw_string_ostream RetneOS(Printed);
  InstPrinter->printInst(&Retne, /*Address=*/0, /*Annot=*/"", *STI, RetneOS);
  RetneOS.flush();
  EXPECT_EQ(Printed, "\tretne\tlr");

  Printed.clear();
  raw_string_ostream RetjOS(Printed);
  InstPrinter->printInst(&Retj, /*Address=*/0, /*Annot=*/"", *STI, RetjOS);
  RetjOS.flush();
  EXPECT_EQ(Printed, "\tretj");

  Printed.clear();
  raw_string_ostream RetsOS(Printed);
  InstPrinter->printInst(&Rets, /*Address=*/0, /*Annot=*/"", *STI, RetsOS);
  RetsOS.flush();
  EXPECT_EQ(Printed, "\trets");

  Printed.clear();
  raw_string_ostream RetssOS(Printed);
  InstPrinter->printInst(&Retss, /*Address=*/0, /*Annot=*/"", *STI, RetssOS);
  RetssOS.flush();
  EXPECT_EQ(Printed, "\tretss");

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
  raw_string_ostream SbcOS(Printed);
  InstPrinter->printInst(&Sbc, /*Address=*/0, /*Annot=*/"", *STI, SbcOS);
  SbcOS.flush();
  EXPECT_EQ(Printed, "\tsbc\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream SbrOS(Printed);
  InstPrinter->printInst(&Sbr, /*Address=*/0, /*Annot=*/"", *STI, SbrOS);
  SbrOS.flush();
  EXPECT_EQ(Printed, "\tsbr\tr1, 1");

  Printed.clear();
  raw_string_ostream ScallOS(Printed);
  InstPrinter->printInst(&Scall, /*Address=*/0, /*Annot=*/"", *STI, ScallOS);
  ScallOS.flush();
  EXPECT_EQ(Printed, "\tscall");

  Printed.clear();
  raw_string_ostream SscallOS(Printed);
  InstPrinter->printInst(&Sscall, /*Address=*/0, /*Annot=*/"", *STI,
                         SscallOS);
  SscallOS.flush();
  EXPECT_EQ(Printed, "\tsscall");

  Printed.clear();
  raw_string_ostream ScrOS(Printed);
  InstPrinter->printInst(&Scr, /*Address=*/0, /*Annot=*/"", *STI, ScrOS);
  ScrOS.flush();
  EXPECT_EQ(Printed, "\tscr\tr1");

  Printed.clear();
  raw_string_ostream SrEqOS(Printed);
  InstPrinter->printInst(&SrEq, /*Address=*/0, /*Annot=*/"", *STI, SrEqOS);
  SrEqOS.flush();
  EXPECT_EQ(Printed, "\tsreq\tr1");

  Printed.clear();
  raw_string_ostream SrAlOS(Printed);
  InstPrinter->printInst(&SrAl, /*Address=*/0, /*Annot=*/"", *STI, SrAlOS);
  SrAlOS.flush();
  EXPECT_EQ(Printed, "\tsral\tr1");

  Printed.clear();
  raw_string_ostream SleepOS(Printed);
  InstPrinter->printInst(&Sleep, /*Address=*/0, /*Annot=*/"", *STI, SleepOS);
  SleepOS.flush();
  EXPECT_EQ(Printed, "\tsleep\t1");

  Printed.clear();
  raw_string_ostream SsrfOS(Printed);
  InstPrinter->printInst(&Ssrf, /*Address=*/0, /*Annot=*/"", *STI, SsrfOS);
  SsrfOS.flush();
  EXPECT_EQ(Printed, "\tssrf\t1");

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
  raw_string_ostream SyncOS(Printed);
  InstPrinter->printInst(&Sync, /*Address=*/0, /*Annot=*/"", *STI, SyncOS);
  SyncOS.flush();
  EXPECT_EQ(Printed, "\tsync\t1");

  Printed.clear();
  raw_string_ostream TlbrOS(Printed);
  InstPrinter->printInst(&Tlbr, /*Address=*/0, /*Annot=*/"", *STI, TlbrOS);
  TlbrOS.flush();
  EXPECT_EQ(Printed, "\ttlbr");

  Printed.clear();
  raw_string_ostream TlbsOS(Printed);
  InstPrinter->printInst(&Tlbs, /*Address=*/0, /*Annot=*/"", *STI, TlbsOS);
  TlbsOS.flush();
  EXPECT_EQ(Printed, "\ttlbs");

  Printed.clear();
  raw_string_ostream TlbwOS(Printed);
  InstPrinter->printInst(&Tlbw, /*Address=*/0, /*Annot=*/"", *STI, TlbwOS);
  TlbwOS.flush();
  EXPECT_EQ(Printed, "\ttlbw");

  Printed.clear();
  raw_string_ostream TnbzOS(Printed);
  InstPrinter->printInst(&Tnbz, /*Address=*/0, /*Annot=*/"", *STI, TnbzOS);
  TnbzOS.flush();
  EXPECT_EQ(Printed, "\ttnbz\tr1");

  Printed.clear();
  raw_string_ostream TstOS(Printed);
  InstPrinter->printInst(&Tst, /*Address=*/0, /*Annot=*/"", *STI, TstOS);
  TstOS.flush();
  EXPECT_EQ(Printed, "\ttst\tr1, r2");

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

  Printed.clear();
  raw_string_ostream MovhOS(Printed);
  InstPrinter->printInst(&Movh, /*Address=*/0, /*Annot=*/"", *STI, MovhOS);
  MovhOS.flush();
  EXPECT_EQ(Printed, "\tmovh\tr1, 1");

  SourceMgr SrcMgr;
  SrcMgr.AddNewSourceBuffer(
      MemoryBuffer::getMemBuffer(
          "nop\nfrs\nabs r1\nacr r1\nacall 4\nadc r1, r2, r3\naddabs r1, r2, r3\nadd r1, r2\nand r1, r2\nandn r1, r2\nasr r1, r2, r3\nbld r1, 1\nbrev r1\nbst r1, 1\nbreakpoint\ncbr r1, 1\ncasts.b r1\ncasts.h r1\ncastu.b r1\ncastu.h r1\nclz r1, r2\ncom r1\ncpc r1\ncpc r1, r2\ncp.w r1, r2\ncp.w r1, -1\ncp.w r1, 32\ncsrfcz 1\ncsrf 1\nneg r1\neor r1, r2\nicall r1\nincjosp -1\nlsl r1, r2, r3\nlsr r1, r2, r3\nmax r1, r2, r3\nmin r1, r2, r3\nmfdr r1, 4\nmfsr r1, 4\nmul r1, r2\nmul r1, r2, -1\nmul r1, r2, r3\nmuls.d r2, r3, r4\nmulu.d r2, r3, r4\nmusfr r1\nmustr r1\nmtdr 4, r1\nmtsr 4, r1\nor r1, r2\npopjc\npushjc\nretd\nrete\nret\nretal lr\nretcc lr\nretcs lr\nretlo lr\nretge lr\nretlt lr\nretmi lr\nretpl lr\nretls lr\nretgt lr\nretle lr\nrethi lr\nretvs lr\nretvc lr\nretqs lr\nreths lr\nreteq lr\nretne lr\nretj\nrets\nretss\nrol r1\nror r1\nrsub r1, r2\nsbc r1, r2, r3\nsbr r1, 1\nscall\nsscall\nscr r1\nsral r1\nsrcc r1\nsrcs r1\nsreq r1\nsrge r1\nsrgt r1\nsrhi r1\nsrhs r1\nsrle r1\nsrlo r1\nsrls r1\nsrlt r1\nsrmi r1\nsrne r1\nsrpl r1\nsrqs r1\nsrvc r1\nsrvs r1\nsleep 1\nssrf 1\nsub r1, r2\nswap.bh r1\nswap.b r1\nswap.h r1\nsync 1\ntlbr\ntlbs\ntlbw\ntnbz r1\ntst r1, r2\nmov r1, r2\nmov r1, -1\nmovh r1, 1\n"),
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
