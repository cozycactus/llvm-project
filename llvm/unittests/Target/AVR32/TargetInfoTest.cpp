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
  EXPECT_EQ(MII->get(AVR32::ADDALrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDCCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDCSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDEQrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDGErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDGTrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDHIrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDHSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDLErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDLOrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDLSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDLTrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDMIrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDNErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDPLrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDQSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDVCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDVSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ADDrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ANDHri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDHri_COH).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDLri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDLri_COH).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDALrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDCCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDCSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDEQrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDGErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDGTrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDHIrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDHSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDLErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDLOrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDLSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDLTrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDMIrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDNErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDPLrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDQSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDVCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDVSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ANDNrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ANDrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ASRrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::BFEXTS).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::BFEXTU).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::BFINS).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::BLDri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::BREVr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::BSTri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::BREAKPOINT).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CBRri).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CACHE).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::CASTS_Br).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CASTS_Hr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CASTU_Br).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CASTU_Hr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CLZrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::COMr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CPCr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CPCrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::CP_Brr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::CP_Hrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::CP_Wri21).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::CP_Wri6).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CP_Wrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CSRFCZi).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::CSRFi).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::DIVSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::DIVUrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORHri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORLri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORALrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORCCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORCSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EOREQrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORGErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORGTrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORHIrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORHSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORLErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORLOrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORLSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORLTrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORMIrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORNErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORPLrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORQSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORVCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::EORVSrrr).getSize(), 4u);
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
  EXPECT_EQ(MII->get(AVR32::MOVALri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVCCri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVCSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVEQri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVGEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVGTri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVHIri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVHSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVLEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVLOri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVLSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVLTri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVMIri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVNEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVPLri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVQSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVVCri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVVSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVALrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVCCrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVCSrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVEQrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVGErr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVGTrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVHIrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVHSrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVLErr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVLOrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVLSrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVLTrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVMIrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVNErr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVPLrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVQSrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVVCrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVVSrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVri8).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::MOVri21).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MOVHri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MEMC).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MEMS).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::MEMT).getSize(), 4u);
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
  EXPECT_EQ(MII->get(AVR32::ORHri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORLri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORALrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORCCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORCSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::OREQrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORGErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORGTrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORHIrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORHSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORLErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORLOrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORLSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORLTrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORMIrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORNErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORPLrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORQSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORVCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORVSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ORrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::PABS_SBrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PABS_SHrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PACKSH_UBrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PACKSH_SBrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PACKW_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADD_Brrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADD_Hrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDH_UBrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDH_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDS_UBrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDS_SBrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDS_UHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDS_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDSUB_H).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDSUBH_SH).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDSUBS_UH).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDSUBS_SH).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDX_Hrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDXH_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDXS_UHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PADDXS_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PASR_Brri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PASR_Hrri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PLSL_Brri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PLSL_Hrri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PLSR_Brri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PLSR_Hrri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PAVG_UBrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PAVG_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PREF).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PMAX_UBrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PMAX_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PMIN_UBrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PMIN_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSADrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUB_Brrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUB_Hrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBADD_H).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBADDH_SH).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBADDS_UH).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBADDS_SH).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBH_UBrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBH_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBS_UBrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBS_SBrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBS_UHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBS_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBX_Hrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBXH_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBXS_UHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PSUBXS_SHrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PUNPCKUB_H).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::PUNPCKSB_H).getSize(), 4u);
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
  EXPECT_EQ(MII->get(AVR32::RSUBALri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBCCri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBCSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBEQri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBGEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBGTri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBHIri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBHSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBLEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBLOri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBLSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBLTri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBMIri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBNEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBPLri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBQSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBVCri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBVSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::RSUBrr).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SATADD_Wrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SATRNDSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SATRNDUri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SATSUB_Hrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SATSUB_Wrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SATSUB_Wrri16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SATSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SATUri).getSize(), 4u);
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
  EXPECT_EQ(MII->get(AVR32::LD_W_PostInc).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_W_PreDec).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_W_Disp5).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_W_Disp16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_IndexShift).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_IndexPart).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_AL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_EQ_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_NE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_CC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_HS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_CS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_LO_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_GE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_LT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_MI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_PL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_LS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_GT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_LE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_HI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_VS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_VC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_W_QS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_D_PostInc).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_D_PreDec).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_D_Reg).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_D_Disp16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_D_IndexShift).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LDDPC).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LDDSP).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LDINS_B).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LDINS_H).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_Disp16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_IndexShift).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_AL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_EQ_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_NE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_CC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_HS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_CS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_LO_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_GE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_LT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_MI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_PL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_LS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_GT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_LE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_HI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_VS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_VC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SB_QS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_PostInc).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_PreDec).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_Disp3).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_Disp16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_IndexShift).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_AL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_EQ_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_NE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_CC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_HS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_CS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_LO_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_GE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_LT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_MI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_PL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_LS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_GT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_LE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_HI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_VS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_VC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UB_QS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_PostInc).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_PreDec).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_Disp3).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_Disp16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_IndexShift).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_AL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_EQ_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_NE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_CC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_HS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_CS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_LO_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_GE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_LT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_MI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_PL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_LS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_GT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_LE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_HI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_VS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_VC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_SH_QS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_PostInc).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_PreDec).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_Disp3).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_Disp16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_IndexShift).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_AL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_EQ_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_NE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_CC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_HS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_CS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_LO_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_GE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_LT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_MI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_PL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_LS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_GT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_LE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_HI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_VS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_VC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::LD_UH_QS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_PostInc).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_B_PreDec).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_B_Disp3).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_B_Disp16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_IndexShift).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_AL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_EQ_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_NE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_CC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_HS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_CS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_LO_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_GE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_LT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_MI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_PL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_LS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_GT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_LE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_HI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_VS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_VC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_B_QS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_D_PostInc).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_D_PreDec).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_D_Reg).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_D_Disp16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_D_IndexShift).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_PostInc).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_H_PreDec).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_H_Disp3).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_H_Disp16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_IndexShift).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_AL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_EQ_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_NE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_CC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_HS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_CS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_LO_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_GE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_LT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_MI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_PL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_LS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_GT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_LE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_HI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_VS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_VC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_H_QS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_PostInc).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_W_PreDec).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_W_Disp4).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::ST_W_Disp16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_IndexShift).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_AL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_EQ_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_NE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_CC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_HS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_CS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_LO_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_GE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_LT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_MI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_PL_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_LS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_GT_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_LE_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_HI_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_VS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_VC_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::ST_W_QS_Disp9).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::STCOND).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::STDSP).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::STHH_W_Disp8).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::STHH_W_IndexShift).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::STSWP_H_Disp12).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::STSWP_W_Disp12).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBALrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBCCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBCSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBEQrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBGErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBGTrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBHIrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBHSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBLErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBLOrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBLSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBLTrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBMIrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBNErrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBPLrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBQSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBVCrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBVSrrr).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBALri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBCCri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBCSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBEQri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBGEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBGTri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBHIri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBHSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBLEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBLOri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBLSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBLTri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBMIri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBNEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBPLri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBQSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBVCri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBVSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFALri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFCCri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFCSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFEQri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFGEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFGTri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFHIri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFHSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFLEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFLOri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFLSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFLTri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFMIri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFNEri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFPLri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFQSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFVCri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBFVSri).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBHH_W).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBSPri8).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SUBri8).getSize(), 2u);
  EXPECT_EQ(MII->get(AVR32::SUBrrrs).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBrri16).getSize(), 4u);
  EXPECT_EQ(MII->get(AVR32::SUBri21).getSize(), 4u);
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
  EXPECT_EQ(MII->get(AVR32::XCHGrrr).getSize(), 4u);

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

  MCInst AddEq;
  AddEq.setOpcode(AVR32::ADDEQrrr);
  AddEq.addOperand(MCOperand::createReg(AVR32::R1));
  AddEq.addOperand(MCOperand::createReg(AVR32::R2));
  AddEq.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(AddEq, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xe0);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst AddAl;
  AddAl.setOpcode(AVR32::ADDALrrr);
  AddAl.addOperand(MCOperand::createReg(AVR32::R1));
  AddAl.addOperand(MCOperand::createReg(AVR32::R2));
  AddAl.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(AddAl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xef);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

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

  MCInst Andh;
  Andh.setOpcode(AVR32::ANDHri);
  Andh.addOperand(MCOperand::createReg(AVR32::R1));
  Andh.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Andh, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst AndhCOH;
  AndhCOH.setOpcode(AVR32::ANDHri_COH);
  AndhCOH.addOperand(MCOperand::createReg(AVR32::R1));
  AndhCOH.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(AndhCOH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst Andl;
  Andl.setOpcode(AVR32::ANDLri);
  Andl.addOperand(MCOperand::createReg(AVR32::R1));
  Andl.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Andl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe0);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst AndlCOH;
  AndlCOH.setOpcode(AVR32::ANDLri_COH);
  AndlCOH.addOperand(MCOperand::createReg(AVR32::R1));
  AndlCOH.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(AndlCOH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst AndEq;
  AndEq.setOpcode(AVR32::ANDEQrrr);
  AndEq.addOperand(MCOperand::createReg(AVR32::R1));
  AndEq.addOperand(MCOperand::createReg(AVR32::R2));
  AndEq.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(AndEq, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xe0);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst AndAl;
  AndAl.setOpcode(AVR32::ANDALrrr);
  AndAl.addOperand(MCOperand::createReg(AVR32::R1));
  AndAl.addOperand(MCOperand::createReg(AVR32::R2));
  AndAl.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(AndAl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xef);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

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

  MCInst Bfexts;
  Bfexts.setOpcode(AVR32::BFEXTS);
  Bfexts.addOperand(MCOperand::createReg(AVR32::R1));
  Bfexts.addOperand(MCOperand::createReg(AVR32::R2));
  Bfexts.addOperand(MCOperand::createImm(3));
  Bfexts.addOperand(MCOperand::createImm(5));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Bfexts, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xb0);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x65);

  MCInst Bfextu;
  Bfextu.setOpcode(AVR32::BFEXTU);
  Bfextu.addOperand(MCOperand::createReg(AVR32::R1));
  Bfextu.addOperand(MCOperand::createReg(AVR32::R2));
  Bfextu.addOperand(MCOperand::createImm(3));
  Bfextu.addOperand(MCOperand::createImm(5));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Bfextu, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xc0);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x65);

  MCInst Bfins;
  Bfins.setOpcode(AVR32::BFINS);
  Bfins.addOperand(MCOperand::createReg(AVR32::R1));
  Bfins.addOperand(MCOperand::createReg(AVR32::R2));
  Bfins.addOperand(MCOperand::createImm(3));
  Bfins.addOperand(MCOperand::createImm(5));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Bfins, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xd0);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x65);

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

  MCInst Cache;
  Cache.setOpcode(AVR32::CACHE);
  Cache.addOperand(MCOperand::createReg(AVR32::R1));
  Cache.addOperand(MCOperand::createImm(-1));
  Cache.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Cache, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0f);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

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

  MCInst CpB;
  CpB.setOpcode(AVR32::CP_Brr);
  CpB.addOperand(MCOperand::createReg(AVR32::R1));
  CpB.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(CpB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x18);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x00);

  MCInst CpH;
  CpH.setOpcode(AVR32::CP_Hrr);
  CpH.addOperand(MCOperand::createReg(AVR32::R1));
  CpH.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(CpH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x19);
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

  MCInst Eorh;
  Eorh.setOpcode(AVR32::EORHri);
  Eorh.addOperand(MCOperand::createReg(AVR32::R1));
  Eorh.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Eorh, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xee);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst Eorl;
  Eorl.setOpcode(AVR32::EORLri);
  Eorl.addOperand(MCOperand::createReg(AVR32::R1));
  Eorl.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Eorl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xec);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst EorEq;
  EorEq.setOpcode(AVR32::EOREQrrr);
  EorEq.addOperand(MCOperand::createReg(AVR32::R1));
  EorEq.addOperand(MCOperand::createReg(AVR32::R2));
  EorEq.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(EorEq, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xe0);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst EorAl;
  EorAl.setOpcode(AVR32::EORALrrr);
  EorAl.addOperand(MCOperand::createReg(AVR32::R1));
  EorAl.addOperand(MCOperand::createReg(AVR32::R2));
  EorAl.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(EorAl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xef);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

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

  MCInst LdWPostInc;
  LdWPostInc.setOpcode(AVR32::LD_W_PostInc);
  LdWPostInc.addOperand(MCOperand::createReg(AVR32::R1));
  LdWPostInc.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdWPostInc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x05);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);

  MCInst LdWPreDec;
  LdWPreDec.setOpcode(AVR32::LD_W_PreDec);
  LdWPreDec.addOperand(MCOperand::createReg(AVR32::R1));
  LdWPreDec.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdWPreDec, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x05);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x41);

  MCInst LdWDisp5;
  LdWDisp5.setOpcode(AVR32::LD_W_Disp5);
  LdWDisp5.addOperand(MCOperand::createReg(AVR32::R1));
  LdWDisp5.addOperand(MCOperand::createReg(AVR32::R2));
  LdWDisp5.addOperand(MCOperand::createImm(12));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdWDisp5, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x64);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x31);

  MCInst LdWDisp16;
  LdWDisp16.setOpcode(AVR32::LD_W_Disp16);
  LdWDisp16.addOperand(MCOperand::createReg(AVR32::R1));
  LdWDisp16.addOperand(MCOperand::createReg(AVR32::R2));
  LdWDisp16.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdWDisp16, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst LdWIndexShift;
  LdWIndexShift.setOpcode(AVR32::LD_W_IndexShift);
  LdWIndexShift.addOperand(MCOperand::createReg(AVR32::R1));
  LdWIndexShift.addOperand(MCOperand::createReg(AVR32::R2));
  LdWIndexShift.addOperand(MCOperand::createReg(AVR32::R3));
  LdWIndexShift.addOperand(MCOperand::createImm(2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdWIndexShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst LdWIndexPart;
  LdWIndexPart.setOpcode(AVR32::LD_W_IndexPart);
  LdWIndexPart.addOperand(MCOperand::createReg(AVR32::R1));
  LdWIndexPart.addOperand(MCOperand::createReg(AVR32::R2));
  LdWIndexPart.addOperand(MCOperand::createReg(AVR32::R3));
  LdWIndexPart.addOperand(MCOperand::createImm(1));
  LdWIndexPart.addOperand(MCOperand::createImm(2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdWIndexPart, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0f);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x91);

  MCInst LdWEqDisp9;
  LdWEqDisp9.setOpcode(AVR32::LD_W_EQ_Disp9);
  LdWEqDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  LdWEqDisp9.addOperand(MCOperand::createReg(AVR32::R2));
  LdWEqDisp9.addOperand(MCOperand::createImm(12));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdWEqDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst LdWAlDisp9;
  LdWAlDisp9.setOpcode(AVR32::LD_W_AL_Disp9);
  LdWAlDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  LdWAlDisp9.addOperand(MCOperand::createReg(AVR32::R2));
  LdWAlDisp9.addOperand(MCOperand::createImm(12));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdWAlDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xf0);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst LdDPostInc;
  LdDPostInc.setOpcode(AVR32::LD_D_PostInc);
  LdDPostInc.addOperand(MCOperand::createReg(AVR32::R2));
  LdDPostInc.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdDPostInc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xa3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);

  MCInst LdDPreDec;
  LdDPreDec.setOpcode(AVR32::LD_D_PreDec);
  LdDPreDec.addOperand(MCOperand::createReg(AVR32::R2));
  LdDPreDec.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdDPreDec, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xa3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x12);

  MCInst LdDReg;
  LdDReg.setOpcode(AVR32::LD_D_Reg);
  LdDReg.addOperand(MCOperand::createReg(AVR32::R2));
  LdDReg.addOperand(MCOperand::createReg(AVR32::R1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdDReg, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xa3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x02);

  MCInst LdDDisp16;
  LdDDisp16.setOpcode(AVR32::LD_D_Disp16);
  LdDDisp16.addOperand(MCOperand::createReg(AVR32::R2));
  LdDDisp16.addOperand(MCOperand::createReg(AVR32::R1));
  LdDDisp16.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdDDisp16, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xe2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst LdDIndexShift;
  LdDIndexShift.setOpcode(AVR32::LD_D_IndexShift);
  LdDIndexShift.addOperand(MCOperand::createReg(AVR32::R2));
  LdDIndexShift.addOperand(MCOperand::createReg(AVR32::R1));
  LdDIndexShift.addOperand(MCOperand::createReg(AVR32::R3));
  LdDIndexShift.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdDIndexShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x32);

  MCInst Lddpc;
  Lddpc.setOpcode(AVR32::LDDPC);
  Lddpc.addOperand(MCOperand::createReg(AVR32::R2));
  Lddpc.addOperand(MCOperand::createReg(AVR32::PC));
  Lddpc.addOperand(MCOperand::createImm(12));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Lddpc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x48);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x32);

  MCInst Lddsp;
  Lddsp.setOpcode(AVR32::LDDSP);
  Lddsp.addOperand(MCOperand::createReg(AVR32::R2));
  Lddsp.addOperand(MCOperand::createReg(AVR32::SP));
  Lddsp.addOperand(MCOperand::createImm(12));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Lddsp, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x40);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x32);

  MCInst LdinsB;
  LdinsB.setOpcode(AVR32::LDINS_B);
  LdinsB.addOperand(MCOperand::createReg(AVR32::R1));
  LdinsB.addOperand(MCOperand::createImm(2));
  LdinsB.addOperand(MCOperand::createReg(AVR32::R2));
  LdinsB.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdinsB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x6f);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst LdinsH;
  LdinsH.setOpcode(AVR32::LDINS_H);
  LdinsH.addOperand(MCOperand::createReg(AVR32::R1));
  LdinsH.addOperand(MCOperand::createImm(1));
  LdinsH.addOperand(MCOperand::createReg(AVR32::R2));
  LdinsH.addOperand(MCOperand::createImm(-2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdinsH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x1f);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst LdSBDisp16;
  LdSBDisp16.setOpcode(AVR32::LD_SB_Disp16);
  LdSBDisp16.addOperand(MCOperand::createReg(AVR32::R1));
  LdSBDisp16.addOperand(MCOperand::createReg(AVR32::R2));
  LdSBDisp16.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdSBDisp16, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x21);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst LdSBIndexShift;
  LdSBIndexShift.setOpcode(AVR32::LD_SB_IndexShift);
  LdSBIndexShift.addOperand(MCOperand::createReg(AVR32::R1));
  LdSBIndexShift.addOperand(MCOperand::createReg(AVR32::R2));
  LdSBIndexShift.addOperand(MCOperand::createReg(AVR32::R3));
  LdSBIndexShift.addOperand(MCOperand::createImm(2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdSBIndexShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x06);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst LdSBEqDisp9;
  LdSBEqDisp9.setOpcode(AVR32::LD_SB_EQ_Disp9);
  LdSBEqDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  LdSBEqDisp9.addOperand(MCOperand::createReg(AVR32::R2));
  LdSBEqDisp9.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdSBEqDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x06);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst LdSBAlDisp9;
  LdSBAlDisp9.setOpcode(AVR32::LD_SB_AL_Disp9);
  LdSBAlDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  LdSBAlDisp9.addOperand(MCOperand::createReg(AVR32::R2));
  LdSBAlDisp9.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdSBAlDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xf6);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst LdUBPostInc;
  LdUBPostInc.setOpcode(AVR32::LD_UB_PostInc);
  LdUBPostInc.addOperand(MCOperand::createReg(AVR32::R1));
  LdUBPostInc.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUBPostInc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x05);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x31);

  MCInst LdUBPreDec;
  LdUBPreDec.setOpcode(AVR32::LD_UB_PreDec);
  LdUBPreDec.addOperand(MCOperand::createReg(AVR32::R1));
  LdUBPreDec.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUBPreDec, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x05);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x71);

  MCInst LdUBDisp3;
  LdUBDisp3.setOpcode(AVR32::LD_UB_Disp3);
  LdUBDisp3.addOperand(MCOperand::createReg(AVR32::R1));
  LdUBDisp3.addOperand(MCOperand::createReg(AVR32::R2));
  LdUBDisp3.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUBDisp3, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x05);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);

  MCInst LdUBDisp16;
  LdUBDisp16.setOpcode(AVR32::LD_UB_Disp16);
  LdUBDisp16.addOperand(MCOperand::createReg(AVR32::R1));
  LdUBDisp16.addOperand(MCOperand::createReg(AVR32::R2));
  LdUBDisp16.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUBDisp16, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x31);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst LdUBIndexShift;
  LdUBIndexShift.setOpcode(AVR32::LD_UB_IndexShift);
  LdUBIndexShift.addOperand(MCOperand::createReg(AVR32::R1));
  LdUBIndexShift.addOperand(MCOperand::createReg(AVR32::R2));
  LdUBIndexShift.addOperand(MCOperand::createReg(AVR32::R3));
  LdUBIndexShift.addOperand(MCOperand::createImm(2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUBIndexShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x07);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst LdUBEqDisp9;
  LdUBEqDisp9.setOpcode(AVR32::LD_UB_EQ_Disp9);
  LdUBEqDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  LdUBEqDisp9.addOperand(MCOperand::createReg(AVR32::R2));
  LdUBEqDisp9.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUBEqDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x08);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst LdUBAlDisp9;
  LdUBAlDisp9.setOpcode(AVR32::LD_UB_AL_Disp9);
  LdUBAlDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  LdUBAlDisp9.addOperand(MCOperand::createReg(AVR32::R2));
  LdUBAlDisp9.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUBAlDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xf8);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst LdSHPostInc;
  LdSHPostInc.setOpcode(AVR32::LD_SH_PostInc);
  LdSHPostInc.addOperand(MCOperand::createReg(AVR32::R1));
  LdSHPostInc.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdSHPostInc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x05);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);

  MCInst LdSHPreDec;
  LdSHPreDec.setOpcode(AVR32::LD_SH_PreDec);
  LdSHPreDec.addOperand(MCOperand::createReg(AVR32::R1));
  LdSHPreDec.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdSHPreDec, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x05);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x51);

  MCInst LdSHDisp3;
  LdSHDisp3.setOpcode(AVR32::LD_SH_Disp3);
  LdSHDisp3.addOperand(MCOperand::createReg(AVR32::R1));
  LdSHDisp3.addOperand(MCOperand::createReg(AVR32::R2));
  LdSHDisp3.addOperand(MCOperand::createImm(6));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdSHDisp3, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x84);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x31);

  MCInst LdSHDisp16;
  LdSHDisp16.setOpcode(AVR32::LD_SH_Disp16);
  LdSHDisp16.addOperand(MCOperand::createReg(AVR32::R1));
  LdSHDisp16.addOperand(MCOperand::createReg(AVR32::R2));
  LdSHDisp16.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdSHDisp16, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst LdSHIndexShift;
  LdSHIndexShift.setOpcode(AVR32::LD_SH_IndexShift);
  LdSHIndexShift.addOperand(MCOperand::createReg(AVR32::R1));
  LdSHIndexShift.addOperand(MCOperand::createReg(AVR32::R2));
  LdSHIndexShift.addOperand(MCOperand::createReg(AVR32::R3));
  LdSHIndexShift.addOperand(MCOperand::createImm(2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdSHIndexShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst LdSHEqDisp9;
  LdSHEqDisp9.setOpcode(AVR32::LD_SH_EQ_Disp9);
  LdSHEqDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  LdSHEqDisp9.addOperand(MCOperand::createReg(AVR32::R2));
  LdSHEqDisp9.addOperand(MCOperand::createImm(6));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdSHEqDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst LdSHAlDisp9;
  LdSHAlDisp9.setOpcode(AVR32::LD_SH_AL_Disp9);
  LdSHAlDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  LdSHAlDisp9.addOperand(MCOperand::createReg(AVR32::R2));
  LdSHAlDisp9.addOperand(MCOperand::createImm(6));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdSHAlDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst LdUHPostInc;
  LdUHPostInc.setOpcode(AVR32::LD_UH_PostInc);
  LdUHPostInc.addOperand(MCOperand::createReg(AVR32::R1));
  LdUHPostInc.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUHPostInc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x05);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x21);

  MCInst LdUHPreDec;
  LdUHPreDec.setOpcode(AVR32::LD_UH_PreDec);
  LdUHPreDec.addOperand(MCOperand::createReg(AVR32::R1));
  LdUHPreDec.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUHPreDec, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x05);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x61);

  MCInst LdUHDisp3;
  LdUHDisp3.setOpcode(AVR32::LD_UH_Disp3);
  LdUHDisp3.addOperand(MCOperand::createReg(AVR32::R1));
  LdUHDisp3.addOperand(MCOperand::createReg(AVR32::R2));
  LdUHDisp3.addOperand(MCOperand::createImm(6));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUHDisp3, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x84);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);

  MCInst LdUHDisp16;
  LdUHDisp16.setOpcode(AVR32::LD_UH_Disp16);
  LdUHDisp16.addOperand(MCOperand::createReg(AVR32::R1));
  LdUHDisp16.addOperand(MCOperand::createReg(AVR32::R2));
  LdUHDisp16.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUHDisp16, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst LdUHIndexShift;
  LdUHIndexShift.setOpcode(AVR32::LD_UH_IndexShift);
  LdUHIndexShift.addOperand(MCOperand::createReg(AVR32::R1));
  LdUHIndexShift.addOperand(MCOperand::createReg(AVR32::R2));
  LdUHIndexShift.addOperand(MCOperand::createReg(AVR32::R3));
  LdUHIndexShift.addOperand(MCOperand::createImm(2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUHIndexShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x05);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst LdUHEqDisp9;
  LdUHEqDisp9.setOpcode(AVR32::LD_UH_EQ_Disp9);
  LdUHEqDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  LdUHEqDisp9.addOperand(MCOperand::createReg(AVR32::R2));
  LdUHEqDisp9.addOperand(MCOperand::createImm(6));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUHEqDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst LdUHAlDisp9;
  LdUHAlDisp9.setOpcode(AVR32::LD_UH_AL_Disp9);
  LdUHAlDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  LdUHAlDisp9.addOperand(MCOperand::createReg(AVR32::R2));
  LdUHAlDisp9.addOperand(MCOperand::createImm(6));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(LdUHAlDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xf4);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

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

  MCInst Divs;
  Divs.setOpcode(AVR32::DIVSrrr);
  Divs.addOperand(MCOperand::createReg(AVR32::R2));
  Divs.addOperand(MCOperand::createReg(AVR32::R3));
  Divs.addOperand(MCOperand::createReg(AVR32::R4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Divs, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0c);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x02);

  MCInst Divu;
  Divu.setOpcode(AVR32::DIVUrrr);
  Divu.addOperand(MCOperand::createReg(AVR32::R2));
  Divu.addOperand(MCOperand::createReg(AVR32::R3));
  Divu.addOperand(MCOperand::createReg(AVR32::R4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Divu, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe6);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0d);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x02);

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

  MCInst Memc;
  Memc.setOpcode(AVR32::MEMC);
  Memc.addOperand(MCOperand::createImm(4));
  Memc.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Memc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xfc);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x10);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x80);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst Mems;
  Mems.setOpcode(AVR32::MEMS);
  Mems.addOperand(MCOperand::createImm(4));
  Mems.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Mems, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf0);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x10);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x80);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst Memt;
  Memt.setOpcode(AVR32::MEMT);
  Memt.addOperand(MCOperand::createImm(4));
  Memt.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Memt, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x10);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x80);
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

  MCInst Orh;
  Orh.setOpcode(AVR32::ORHri);
  Orh.addOperand(MCOperand::createReg(AVR32::R1));
  Orh.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Orh, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xea);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst Orl;
  Orl.setOpcode(AVR32::ORLri);
  Orl.addOperand(MCOperand::createReg(AVR32::R1));
  Orl.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Orl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe8);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst Pref;
  Pref.setOpcode(AVR32::PREF);
  Pref.addOperand(MCOperand::createReg(AVR32::R1));
  Pref.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Pref, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x11);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst PabsSB;
  PabsSB.setOpcode(AVR32::PABS_SBrr);
  PabsSB.addOperand(MCOperand::createReg(AVR32::R1));
  PabsSB.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PabsSB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe0);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xe1);

  MCInst PabsSH;
  PabsSH.setOpcode(AVR32::PABS_SHrr);
  PabsSH.addOperand(MCOperand::createReg(AVR32::R1));
  PabsSH.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PabsSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe0);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xf1);

  MCInst PackshUB;
  PackshUB.setOpcode(AVR32::PACKSH_UBrrr);
  PackshUB.addOperand(MCOperand::createReg(AVR32::R1));
  PackshUB.addOperand(MCOperand::createReg(AVR32::R2));
  PackshUB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PackshUB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xc1);

  MCInst PackshSB;
  PackshSB.setOpcode(AVR32::PACKSH_SBrrr);
  PackshSB.addOperand(MCOperand::createReg(AVR32::R1));
  PackshSB.addOperand(MCOperand::createReg(AVR32::R2));
  PackshSB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PackshSB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xd1);

  MCInst PackwSH;
  PackwSH.setOpcode(AVR32::PACKW_SHrrr);
  PackwSH.addOperand(MCOperand::createReg(AVR32::R1));
  PackwSH.addOperand(MCOperand::createReg(AVR32::R2));
  PackwSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PackwSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x71);

  MCInst PaddB;
  PaddB.setOpcode(AVR32::PADD_Brrr);
  PaddB.addOperand(MCOperand::createReg(AVR32::R1));
  PaddB.addOperand(MCOperand::createReg(AVR32::R2));
  PaddB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst PaddH;
  PaddH.setOpcode(AVR32::PADD_Hrrr);
  PaddH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst PaddhUB;
  PaddhUB.setOpcode(AVR32::PADDH_UBrrr);
  PaddhUB.addOperand(MCOperand::createReg(AVR32::R1));
  PaddhUB.addOperand(MCOperand::createReg(AVR32::R2));
  PaddhUB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddhUB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x61);

  MCInst PaddhSH;
  PaddhSH.setOpcode(AVR32::PADDH_SHrrr);
  PaddhSH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddhSH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddhSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddhSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xc1);

  MCInst PaddsUB;
  PaddsUB.setOpcode(AVR32::PADDS_UBrrr);
  PaddsUB.addOperand(MCOperand::createReg(AVR32::R1));
  PaddsUB.addOperand(MCOperand::createReg(AVR32::R2));
  PaddsUB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddsUB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst PaddsSB;
  PaddsSB.setOpcode(AVR32::PADDS_SBrrr);
  PaddsSB.addOperand(MCOperand::createReg(AVR32::R1));
  PaddsSB.addOperand(MCOperand::createReg(AVR32::R2));
  PaddsSB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddsSB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst PaddsUH;
  PaddsUH.setOpcode(AVR32::PADDS_UHrrr);
  PaddsUH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddsUH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddsUH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddsUH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x81);

  MCInst PaddsSH;
  PaddsSH.setOpcode(AVR32::PADDS_SHrrr);
  PaddsSH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddsSH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddsSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddsSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst PaddsubH;
  PaddsubH.setOpcode(AVR32::PADDSUB_H);
  PaddsubH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddsubH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddsubH.addOperand(MCOperand::createImm(1));
  PaddsubH.addOperand(MCOperand::createReg(AVR32::R3));
  PaddsubH.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddsubH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x21);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst PaddsubhSH;
  PaddsubhSH.setOpcode(AVR32::PADDSUBH_SH);
  PaddsubhSH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddsubhSH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddsubhSH.addOperand(MCOperand::createImm(1));
  PaddsubhSH.addOperand(MCOperand::createReg(AVR32::R3));
  PaddsubhSH.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddsubhSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x22);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xa1);

  MCInst PaddsubsUH;
  PaddsubsUH.setOpcode(AVR32::PADDSUBS_UH);
  PaddsubsUH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddsubsUH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddsubsUH.addOperand(MCOperand::createImm(1));
  PaddsubsUH.addOperand(MCOperand::createReg(AVR32::R3));
  PaddsubsUH.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddsubsUH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x22);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst PaddsubsSH;
  PaddsubsSH.setOpcode(AVR32::PADDSUBS_SH);
  PaddsubsSH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddsubsSH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddsubsSH.addOperand(MCOperand::createImm(1));
  PaddsubsSH.addOperand(MCOperand::createReg(AVR32::R3));
  PaddsubsSH.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddsubsSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x21);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xa1);

  MCInst PaddxH;
  PaddxH.setOpcode(AVR32::PADDX_Hrrr);
  PaddxH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddxH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddxH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddxH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst PaddxhSH;
  PaddxhSH.setOpcode(AVR32::PADDXH_SHrrr);
  PaddxhSH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddxhSH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddxhSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddxhSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xe1);

  MCInst PaddxsUH;
  PaddxsUH.setOpcode(AVR32::PADDXS_UHrrr);
  PaddxsUH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddxsUH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddxsUH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddxsUH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xa1);

  MCInst PaddxsSH;
  PaddxsSH.setOpcode(AVR32::PADDXS_SHrrr);
  PaddxsSH.addOperand(MCOperand::createReg(AVR32::R1));
  PaddxsSH.addOperand(MCOperand::createReg(AVR32::R2));
  PaddxsSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PaddxsSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x61);

  MCInst PasrB;
  PasrB.setOpcode(AVR32::PASR_Brri);
  PasrB.addOperand(MCOperand::createReg(AVR32::R1));
  PasrB.addOperand(MCOperand::createReg(AVR32::R2));
  PasrB.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PasrB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x11);

  MCInst PasrH;
  PasrH.setOpcode(AVR32::PASR_Hrri);
  PasrH.addOperand(MCOperand::createReg(AVR32::R1));
  PasrH.addOperand(MCOperand::createReg(AVR32::R2));
  PasrH.addOperand(MCOperand::createImm(15));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PasrH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x0f);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

  MCInst PlslB;
  PlslB.setOpcode(AVR32::PLSL_Brri);
  PlslB.addOperand(MCOperand::createReg(AVR32::R1));
  PlslB.addOperand(MCOperand::createReg(AVR32::R2));
  PlslB.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PlslB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst PlslH;
  PlslH.setOpcode(AVR32::PLSL_Hrri);
  PlslH.addOperand(MCOperand::createReg(AVR32::R1));
  PlslH.addOperand(MCOperand::createReg(AVR32::R2));
  PlslH.addOperand(MCOperand::createImm(15));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PlslH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x0f);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x51);

  MCInst PlsrB;
  PlsrB.setOpcode(AVR32::PLSR_Brri);
  PlsrB.addOperand(MCOperand::createReg(AVR32::R1));
  PlsrB.addOperand(MCOperand::createReg(AVR32::R2));
  PlsrB.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PlsrB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x31);

  MCInst PlsrH;
  PlsrH.setOpcode(AVR32::PLSR_Hrri);
  PlsrH.addOperand(MCOperand::createReg(AVR32::R1));
  PlsrH.addOperand(MCOperand::createReg(AVR32::R2));
  PlsrH.addOperand(MCOperand::createImm(15));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PlsrH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x0f);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x61);

  MCInst PavgUB;
  PavgUB.setOpcode(AVR32::PAVG_UBrrr);
  PavgUB.addOperand(MCOperand::createReg(AVR32::R1));
  PavgUB.addOperand(MCOperand::createReg(AVR32::R2));
  PavgUB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PavgUB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xc1);

  MCInst PavgSH;
  PavgSH.setOpcode(AVR32::PAVG_SHrrr);
  PavgSH.addOperand(MCOperand::createReg(AVR32::R1));
  PavgSH.addOperand(MCOperand::createReg(AVR32::R2));
  PavgSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PavgSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xd1);

  MCInst PmaxUB;
  PmaxUB.setOpcode(AVR32::PMAX_UBrrr);
  PmaxUB.addOperand(MCOperand::createReg(AVR32::R1));
  PmaxUB.addOperand(MCOperand::createReg(AVR32::R2));
  PmaxUB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PmaxUB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x81);

  MCInst PmaxSH;
  PmaxSH.setOpcode(AVR32::PMAX_SHrrr);
  PmaxSH.addOperand(MCOperand::createReg(AVR32::R1));
  PmaxSH.addOperand(MCOperand::createReg(AVR32::R2));
  PmaxSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PmaxSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x91);

  MCInst PminUB;
  PminUB.setOpcode(AVR32::PMIN_UBrrr);
  PminUB.addOperand(MCOperand::createReg(AVR32::R1));
  PminUB.addOperand(MCOperand::createReg(AVR32::R2));
  PminUB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PminUB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xa1);

  MCInst PminSH;
  PminSH.setOpcode(AVR32::PMIN_SHrrr);
  PminSH.addOperand(MCOperand::createReg(AVR32::R1));
  PminSH.addOperand(MCOperand::createReg(AVR32::R2));
  PminSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PminSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xb1);

  MCInst Psad;
  Psad.setOpcode(AVR32::PSADrrr);
  Psad.addOperand(MCOperand::createReg(AVR32::R1));
  Psad.addOperand(MCOperand::createReg(AVR32::R2));
  Psad.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Psad, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x01);

  MCInst PsubB;
  PsubB.setOpcode(AVR32::PSUB_Brrr);
  PsubB.addOperand(MCOperand::createReg(AVR32::R1));
  PsubB.addOperand(MCOperand::createReg(AVR32::R2));
  PsubB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x11);

  MCInst PsubH;
  PsubH.setOpcode(AVR32::PSUB_Hrrr);
  PsubH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x11);

  MCInst PsubaddH;
  PsubaddH.setOpcode(AVR32::PSUBADD_H);
  PsubaddH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubaddH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubaddH.addOperand(MCOperand::createImm(1));
  PsubaddH.addOperand(MCOperand::createReg(AVR32::R3));
  PsubaddH.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubaddH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x21);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x61);

  MCInst PsubaddhSH;
  PsubaddhSH.setOpcode(AVR32::PSUBADDH_SH);
  PsubaddhSH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubaddhSH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubaddhSH.addOperand(MCOperand::createImm(1));
  PsubaddhSH.addOperand(MCOperand::createReg(AVR32::R3));
  PsubaddhSH.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubaddhSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x22);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xe1);

  MCInst PsubaddsUH;
  PsubaddsUH.setOpcode(AVR32::PSUBADDS_UH);
  PsubaddsUH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubaddsUH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubaddsUH.addOperand(MCOperand::createImm(1));
  PsubaddsUH.addOperand(MCOperand::createReg(AVR32::R3));
  PsubaddsUH.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubaddsUH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x22);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x61);

  MCInst PsubaddsSH;
  PsubaddsSH.setOpcode(AVR32::PSUBADDS_SH);
  PsubaddsSH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubaddsSH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubaddsSH.addOperand(MCOperand::createImm(1));
  PsubaddsSH.addOperand(MCOperand::createReg(AVR32::R3));
  PsubaddsSH.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubaddsSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x21);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xe1);

  MCInst PsubhUB;
  PsubhUB.setOpcode(AVR32::PSUBH_UBrrr);
  PsubhUB.addOperand(MCOperand::createReg(AVR32::R1));
  PsubhUB.addOperand(MCOperand::createReg(AVR32::R2));
  PsubhUB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubhUB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x71);

  MCInst PsubhSH;
  PsubhSH.setOpcode(AVR32::PSUBH_SHrrr);
  PsubhSH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubhSH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubhSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubhSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xd1);

  MCInst PsubsUB;
  PsubsUB.setOpcode(AVR32::PSUBS_UBrrr);
  PsubsUB.addOperand(MCOperand::createReg(AVR32::R1));
  PsubsUB.addOperand(MCOperand::createReg(AVR32::R2));
  PsubsUB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubsUB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x51);

  MCInst PsubsSB;
  PsubsSB.setOpcode(AVR32::PSUBS_SBrrr);
  PsubsSB.addOperand(MCOperand::createReg(AVR32::R1));
  PsubsSB.addOperand(MCOperand::createReg(AVR32::R2));
  PsubsSB.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubsSB, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x23);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x31);

  MCInst PsubsUH;
  PsubsUH.setOpcode(AVR32::PSUBS_UHrrr);
  PsubsUH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubsUH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubsUH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubsUH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x91);

  MCInst PsubsSH;
  PsubsSH.setOpcode(AVR32::PSUBS_SHrrr);
  PsubsSH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubsSH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubsSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubsSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x51);

  MCInst PsubxH;
  PsubxH.setOpcode(AVR32::PSUBX_Hrrr);
  PsubxH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubxH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubxH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubxH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x31);

  MCInst PsubxhSH;
  PsubxhSH.setOpcode(AVR32::PSUBXH_SHrrr);
  PsubxhSH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubxhSH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubxhSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubxhSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xf1);

  MCInst PsubxsUH;
  PsubxsUH.setOpcode(AVR32::PSUBXS_UHrrr);
  PsubxsUH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubxsUH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubxsUH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubxsUH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xb1);

  MCInst PsubxsSH;
  PsubxsSH.setOpcode(AVR32::PSUBXS_SHrrr);
  PsubxsSH.addOperand(MCOperand::createReg(AVR32::R1));
  PsubxsSH.addOperand(MCOperand::createReg(AVR32::R2));
  PsubxsSH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PsubxsSH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x20);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x71);

  MCInst PunpckubH;
  PunpckubH.setOpcode(AVR32::PUNPCKUB_H);
  PunpckubH.addOperand(MCOperand::createReg(AVR32::R1));
  PunpckubH.addOperand(MCOperand::createReg(AVR32::R2));
  PunpckubH.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PunpckubH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x81);

  MCInst PunpcksbH;
  PunpcksbH.setOpcode(AVR32::PUNPCKSB_H);
  PunpcksbH.addOperand(MCOperand::createReg(AVR32::R1));
  PunpcksbH.addOperand(MCOperand::createReg(AVR32::R2));
  PunpcksbH.addOperand(MCOperand::createImm(1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(PunpcksbH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x24);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xb1);

  MCInst OrEq;
  OrEq.setOpcode(AVR32::OREQrrr);
  OrEq.addOperand(MCOperand::createReg(AVR32::R1));
  OrEq.addOperand(MCOperand::createReg(AVR32::R2));
  OrEq.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(OrEq, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xe0);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x31);

  MCInst OrAl;
  OrAl.setOpcode(AVR32::ORALrrr);
  OrAl.addOperand(MCOperand::createReg(AVR32::R1));
  OrAl.addOperand(MCOperand::createReg(AVR32::R2));
  OrAl.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(OrAl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xef);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x31);

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

  MCInst RsubEq;
  RsubEq.setOpcode(AVR32::RSUBEQri);
  RsubEq.addOperand(MCOperand::createReg(AVR32::R1));
  RsubEq.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(RsubEq, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xfb);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst RsubAl;
  RsubAl.setOpcode(AVR32::RSUBALri);
  RsubAl.addOperand(MCOperand::createReg(AVR32::R1));
  RsubAl.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(RsubAl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xfb);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0f);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst SataddW;
  SataddW.setOpcode(AVR32::SATADD_Wrrr);
  SataddW.addOperand(MCOperand::createReg(AVR32::R1));
  SataddW.addOperand(MCOperand::createReg(AVR32::R2));
  SataddW.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SataddW, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xc1);

  MCInst SatsubH;
  SatsubH.setOpcode(AVR32::SATSUB_Hrrr);
  SatsubH.addOperand(MCOperand::createReg(AVR32::R1));
  SatsubH.addOperand(MCOperand::createReg(AVR32::R2));
  SatsubH.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SatsubH, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xc1);

  MCInst SatsubW;
  SatsubW.setOpcode(AVR32::SATSUB_Wrrr);
  SatsubW.addOperand(MCOperand::createReg(AVR32::R1));
  SatsubW.addOperand(MCOperand::createReg(AVR32::R2));
  SatsubW.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SatsubW, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xc1);

  MCInst SatsubWImm;
  SatsubWImm.setOpcode(AVR32::SATSUB_Wrri16);
  SatsubWImm.addOperand(MCOperand::createReg(AVR32::R1));
  SatsubWImm.addOperand(MCOperand::createReg(AVR32::R2));
  SatsubWImm.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SatsubWImm, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst Satrnds;
  Satrnds.setOpcode(AVR32::SATRNDSri);
  Satrnds.addOperand(MCOperand::createReg(AVR32::R1));
  Satrnds.addOperand(MCOperand::createImm(2));
  Satrnds.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Satrnds, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x62);

  MCInst Satrndu;
  Satrndu.setOpcode(AVR32::SATRNDUri);
  Satrndu.addOperand(MCOperand::createReg(AVR32::R1));
  Satrndu.addOperand(MCOperand::createImm(2));
  Satrndu.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Satrndu, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x62);

  MCInst Sats;
  Sats.setOpcode(AVR32::SATSri);
  Sats.addOperand(MCOperand::createReg(AVR32::R1));
  Sats.addOperand(MCOperand::createImm(2));
  Sats.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Sats, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf1);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x62);

  MCInst Satu;
  Satu.setOpcode(AVR32::SATUri);
  Satu.addOperand(MCOperand::createReg(AVR32::R1));
  Satu.addOperand(MCOperand::createImm(2));
  Satu.addOperand(MCOperand::createImm(3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Satu, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf1);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x04);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x62);

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

  MCInst StBPostInc;
  StBPostInc.setOpcode(AVR32::ST_B_PostInc);
  StBPostInc.addOperand(MCOperand::createReg(AVR32::R1));
  StBPostInc.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StBPostInc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xc2);

  MCInst StBPreDec;
  StBPreDec.setOpcode(AVR32::ST_B_PreDec);
  StBPreDec.addOperand(MCOperand::createReg(AVR32::R1));
  StBPreDec.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StBPreDec, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);

  MCInst StBDisp3;
  StBDisp3.setOpcode(AVR32::ST_B_Disp3);
  StBDisp3.addOperand(MCOperand::createReg(AVR32::R1));
  StBDisp3.addOperand(MCOperand::createImm(3));
  StBDisp3.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StBDisp3, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xa2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb2);

  MCInst StBDisp16;
  StBDisp16.setOpcode(AVR32::ST_B_Disp16);
  StBDisp16.addOperand(MCOperand::createReg(AVR32::R1));
  StBDisp16.addOperand(MCOperand::createImm(-1));
  StBDisp16.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StBDisp16, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x62);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst StBIndexShift;
  StBIndexShift.setOpcode(AVR32::ST_B_IndexShift);
  StBIndexShift.addOperand(MCOperand::createReg(AVR32::R1));
  StBIndexShift.addOperand(MCOperand::createReg(AVR32::R2));
  StBIndexShift.addOperand(MCOperand::createImm(3));
  StBIndexShift.addOperand(MCOperand::createReg(AVR32::R4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StBIndexShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0b);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x34);

  MCInst StBEqDisp9;
  StBEqDisp9.setOpcode(AVR32::ST_B_EQ_Disp9);
  StBEqDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  StBEqDisp9.addOperand(MCOperand::createImm(3));
  StBEqDisp9.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StBEqDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0e);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst StBAlDisp9;
  StBAlDisp9.setOpcode(AVR32::ST_B_AL_Disp9);
  StBAlDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  StBAlDisp9.addOperand(MCOperand::createImm(3));
  StBAlDisp9.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StBAlDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xfe);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst StDPostInc;
  StDPostInc.setOpcode(AVR32::ST_D_PostInc);
  StDPostInc.addOperand(MCOperand::createReg(AVR32::R1));
  StDPostInc.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StDPostInc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xa3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x22);

  MCInst StDPreDec;
  StDPreDec.setOpcode(AVR32::ST_D_PreDec);
  StDPreDec.addOperand(MCOperand::createReg(AVR32::R1));
  StDPreDec.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StDPreDec, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xa3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x23);

  MCInst StDReg;
  StDReg.setOpcode(AVR32::ST_D_Reg);
  StDReg.addOperand(MCOperand::createReg(AVR32::R1));
  StDReg.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StDReg, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xa3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x13);

  MCInst StDDisp16;
  StDDisp16.setOpcode(AVR32::ST_D_Disp16);
  StDDisp16.addOperand(MCOperand::createReg(AVR32::R1));
  StDDisp16.addOperand(MCOperand::createImm(-1));
  StDDisp16.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StDDisp16, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst StDIndexShift;
  StDIndexShift.setOpcode(AVR32::ST_D_IndexShift);
  StDIndexShift.addOperand(MCOperand::createReg(AVR32::R1));
  StDIndexShift.addOperand(MCOperand::createReg(AVR32::R2));
  StDIndexShift.addOperand(MCOperand::createImm(3));
  StDIndexShift.addOperand(MCOperand::createReg(AVR32::R4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StDIndexShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x08);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x34);

  MCInst StHPostInc;
  StHPostInc.setOpcode(AVR32::ST_H_PostInc);
  StHPostInc.addOperand(MCOperand::createReg(AVR32::R1));
  StHPostInc.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StHPostInc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb2);

  MCInst StHPreDec;
  StHPreDec.setOpcode(AVR32::ST_H_PreDec);
  StHPreDec.addOperand(MCOperand::createReg(AVR32::R1));
  StHPreDec.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StHPreDec, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xe2);

  MCInst StHDisp3;
  StHDisp3.setOpcode(AVR32::ST_H_Disp3);
  StHDisp3.addOperand(MCOperand::createReg(AVR32::R1));
  StHDisp3.addOperand(MCOperand::createImm(6));
  StHDisp3.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StHDisp3, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xa2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x32);

  MCInst StHDisp16;
  StHDisp16.setOpcode(AVR32::ST_H_Disp16);
  StHDisp16.addOperand(MCOperand::createReg(AVR32::R1));
  StHDisp16.addOperand(MCOperand::createImm(-1));
  StHDisp16.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StHDisp16, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x52);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst StHIndexShift;
  StHIndexShift.setOpcode(AVR32::ST_H_IndexShift);
  StHIndexShift.addOperand(MCOperand::createReg(AVR32::R1));
  StHIndexShift.addOperand(MCOperand::createReg(AVR32::R2));
  StHIndexShift.addOperand(MCOperand::createImm(3));
  StHIndexShift.addOperand(MCOperand::createReg(AVR32::R4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StHIndexShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0a);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x34);

  MCInst StHEqDisp9;
  StHEqDisp9.setOpcode(AVR32::ST_H_EQ_Disp9);
  StHEqDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  StHEqDisp9.addOperand(MCOperand::createImm(6));
  StHEqDisp9.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StHEqDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0c);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst StHAlDisp9;
  StHAlDisp9.setOpcode(AVR32::ST_H_AL_Disp9);
  StHAlDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  StHAlDisp9.addOperand(MCOperand::createImm(6));
  StHAlDisp9.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StHAlDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xfc);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst StWPostInc;
  StWPostInc.setOpcode(AVR32::ST_W_PostInc);
  StWPostInc.addOperand(MCOperand::createReg(AVR32::R1));
  StWPostInc.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StWPostInc, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xa2);

  MCInst StWPreDec;
  StWPreDec.setOpcode(AVR32::ST_W_PreDec);
  StWPreDec.addOperand(MCOperand::createReg(AVR32::R1));
  StWPreDec.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StWPreDec, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd2);

  MCInst StWDisp4;
  StWDisp4.setOpcode(AVR32::ST_W_Disp4);
  StWDisp4.addOperand(MCOperand::createReg(AVR32::R1));
  StWDisp4.addOperand(MCOperand::createImm(12));
  StWDisp4.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StWDisp4, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x83);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x32);

  MCInst StWDisp16;
  StWDisp16.setOpcode(AVR32::ST_W_Disp16);
  StWDisp16.addOperand(MCOperand::createReg(AVR32::R1));
  StWDisp16.addOperand(MCOperand::createImm(-1));
  StWDisp16.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StWDisp16, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x42);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst StWIndexShift;
  StWIndexShift.setOpcode(AVR32::ST_W_IndexShift);
  StWIndexShift.addOperand(MCOperand::createReg(AVR32::R1));
  StWIndexShift.addOperand(MCOperand::createReg(AVR32::R2));
  StWIndexShift.addOperand(MCOperand::createImm(3));
  StWIndexShift.addOperand(MCOperand::createReg(AVR32::R4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StWIndexShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe2);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x02);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x09);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x34);

  MCInst StWEqDisp9;
  StWEqDisp9.setOpcode(AVR32::ST_W_EQ_Disp9);
  StWEqDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  StWEqDisp9.addOperand(MCOperand::createImm(12));
  StWEqDisp9.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StWEqDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0a);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst StWAlDisp9;
  StWAlDisp9.setOpcode(AVR32::ST_W_AL_Disp9);
  StWAlDisp9.addOperand(MCOperand::createReg(AVR32::R1));
  StWAlDisp9.addOperand(MCOperand::createImm(12));
  StWAlDisp9.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StWAlDisp9, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xfa);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x03);

  MCInst Stcond;
  Stcond.setOpcode(AVR32::STCOND);
  Stcond.addOperand(MCOperand::createReg(AVR32::R1));
  Stcond.addOperand(MCOperand::createImm(-1));
  Stcond.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Stcond, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x72);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst Stdsp;
  Stdsp.setOpcode(AVR32::STDSP);
  Stdsp.addOperand(MCOperand::createReg(AVR32::SP));
  Stdsp.addOperand(MCOperand::createImm(12));
  Stdsp.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Stdsp, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x50);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x32);

  MCInst SthhWDisp8;
  SthhWDisp8.setOpcode(AVR32::STHH_W_Disp8);
  SthhWDisp8.addOperand(MCOperand::createReg(AVR32::R1));
  SthhWDisp8.addOperand(MCOperand::createImm(12));
  SthhWDisp8.addOperand(MCOperand::createReg(AVR32::R2));
  SthhWDisp8.addOperand(MCOperand::createImm(1));
  SthhWDisp8.addOperand(MCOperand::createReg(AVR32::R3));
  SthhWDisp8.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SthhWDisp8, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xe0);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x31);

  MCInst SthhWIndexShift;
  SthhWIndexShift.setOpcode(AVR32::STHH_W_IndexShift);
  SthhWIndexShift.addOperand(MCOperand::createReg(AVR32::R1));
  SthhWIndexShift.addOperand(MCOperand::createReg(AVR32::R4));
  SthhWIndexShift.addOperand(MCOperand::createImm(3));
  SthhWIndexShift.addOperand(MCOperand::createReg(AVR32::R2));
  SthhWIndexShift.addOperand(MCOperand::createImm(1));
  SthhWIndexShift.addOperand(MCOperand::createReg(AVR32::R3));
  SthhWIndexShift.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SthhWIndexShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xa4);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x31);

  MCInst StswpHDisp12;
  StswpHDisp12.setOpcode(AVR32::STSWP_H_Disp12);
  StswpHDisp12.addOperand(MCOperand::createReg(AVR32::R1));
  StswpHDisp12.addOperand(MCOperand::createImm(-2));
  StswpHDisp12.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StswpHDisp12, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x9f);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst SubhhW;
  SubhhW.setOpcode(AVR32::SUBHH_W);
  SubhhW.addOperand(MCOperand::createReg(AVR32::R1));
  SubhhW.addOperand(MCOperand::createReg(AVR32::R2));
  SubhhW.addOperand(MCOperand::createImm(1));
  SubhhW.addOperand(MCOperand::createReg(AVR32::R3));
  SubhhW.addOperand(MCOperand::createImm(0));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubhhW, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0f);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst StswpWDisp12;
  StswpWDisp12.setOpcode(AVR32::STSWP_W_Disp12);
  StswpWDisp12.addOperand(MCOperand::createReg(AVR32::R1));
  StswpWDisp12.addOperand(MCOperand::createImm(-4));
  StswpWDisp12.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(StswpWDisp12, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe3);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd2);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xaf);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

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

  MCInst SubImm8;
  SubImm8.setOpcode(AVR32::SUBri8);
  SubImm8.addOperand(MCOperand::createReg(AVR32::R1));
  SubImm8.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubImm8, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x2f);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xf1);

  MCInst SubSPImm8;
  SubSPImm8.setOpcode(AVR32::SUBSPri8);
  SubSPImm8.addOperand(MCOperand::createReg(AVR32::SP));
  SubSPImm8.addOperand(MCOperand::createImm(-4));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubSPImm8, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 2u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0x2f);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xfd);

  MCInst SubShift;
  SubShift.setOpcode(AVR32::SUBrrrs);
  SubShift.addOperand(MCOperand::createReg(AVR32::R1));
  SubShift.addOperand(MCOperand::createReg(AVR32::R2));
  SubShift.addOperand(MCOperand::createReg(AVR32::R3));
  SubShift.addOperand(MCOperand::createImm(2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubShift, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x21);

  MCInst SubRegImm16;
  SubRegImm16.setOpcode(AVR32::SUBrri16);
  SubRegImm16.addOperand(MCOperand::createReg(AVR32::R1));
  SubRegImm16.addOperand(MCOperand::createReg(AVR32::R2));
  SubRegImm16.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubRegImm16, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xc1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst SubImm;
  SubImm.setOpcode(AVR32::SUBri21);
  SubImm.addOperand(MCOperand::createReg(AVR32::R1));
  SubImm.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubImm, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xfe);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x31);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xff);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst SubEq;
  SubEq.setOpcode(AVR32::SUBEQrrr);
  SubEq.addOperand(MCOperand::createReg(AVR32::R1));
  SubEq.addOperand(MCOperand::createReg(AVR32::R2));
  SubEq.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubEq, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xe0);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x11);

  MCInst SubAl;
  SubAl.setOpcode(AVR32::SUBALrrr);
  SubAl.addOperand(MCOperand::createReg(AVR32::R1));
  SubAl.addOperand(MCOperand::createReg(AVR32::R2));
  SubAl.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubAl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xd3);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0xef);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x11);

  MCInst SubEqImm;
  SubEqImm.setOpcode(AVR32::SUBEQri);
  SubEqImm.addOperand(MCOperand::createReg(AVR32::R1));
  SubEqImm.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubEqImm, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf7);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst SubAlImm;
  SubAlImm.setOpcode(AVR32::SUBALri);
  SubAlImm.addOperand(MCOperand::createReg(AVR32::R1));
  SubAlImm.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubAlImm, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf7);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0f);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst SubfEqImm;
  SubfEqImm.setOpcode(AVR32::SUBFEQri);
  SubfEqImm.addOperand(MCOperand::createReg(AVR32::R1));
  SubfEqImm.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubfEqImm, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst SubfAlImm;
  SubfAlImm.setOpcode(AVR32::SUBFALri);
  SubfAlImm.addOperand(MCOperand::createReg(AVR32::R1));
  SubfAlImm.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(SubfAlImm, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf5);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0f);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

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

  MCInst Xchg;
  Xchg.setOpcode(AVR32::XCHGrrr);
  Xchg.addOperand(MCOperand::createReg(AVR32::R1));
  Xchg.addOperand(MCOperand::createReg(AVR32::R2));
  Xchg.addOperand(MCOperand::createReg(AVR32::R3));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(Xchg, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x03);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0b);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x41);

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

  MCInst MovEq;
  MovEq.setOpcode(AVR32::MOVEQrr);
  MovEq.addOperand(MCOperand::createReg(AVR32::R1));
  MovEq.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(MovEq, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x17);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x00);

  MCInst MovAl;
  MovAl.setOpcode(AVR32::MOVALrr);
  MovAl.addOperand(MCOperand::createReg(AVR32::R1));
  MovAl.addOperand(MCOperand::createReg(AVR32::R2));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(MovAl, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe4);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x01);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x17);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xf0);

  MCInst MovEqImm;
  MovEqImm.setOpcode(AVR32::MOVEQri);
  MovEqImm.addOperand(MCOperand::createReg(AVR32::R1));
  MovEqImm.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(MovEqImm, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf9);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

  MCInst MovAlImm;
  MovAlImm.setOpcode(AVR32::MOVALri);
  MovAlImm.addOperand(MCOperand::createReg(AVR32::R1));
  MovAlImm.addOperand(MCOperand::createImm(-1));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(MovAlImm, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xf9);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0xb1);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x0f);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0xff);

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

  MCInst MovImm21;
  MovImm21.setOpcode(AVR32::MOVri21);
  MovImm21.addOperand(MCOperand::createReg(AVR32::R1));
  MovImm21.addOperand(MCOperand::createImm(32));

  Code.clear();
  Fixups.clear();
  MCE->encodeInstruction(MovImm21, Code, Fixups, *STI);

  EXPECT_TRUE(Fixups.empty());
  ASSERT_EQ(Code.size(), 4u);
  EXPECT_EQ(static_cast<uint8_t>(Code[0]), 0xe0);
  EXPECT_EQ(static_cast<uint8_t>(Code[1]), 0x61);
  EXPECT_EQ(static_cast<uint8_t>(Code[2]), 0x00);
  EXPECT_EQ(static_cast<uint8_t>(Code[3]), 0x20);

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
  raw_string_ostream AddEqOS(Printed);
  InstPrinter->printInst(&AddEq, /*Address=*/0, /*Annot=*/"", *STI, AddEqOS);
  AddEqOS.flush();
  EXPECT_EQ(Printed, "\taddeq\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream AddAlOS(Printed);
  InstPrinter->printInst(&AddAl, /*Address=*/0, /*Annot=*/"", *STI, AddAlOS);
  AddAlOS.flush();
  EXPECT_EQ(Printed, "\taddal\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream AndOS(Printed);
  InstPrinter->printInst(&And, /*Address=*/0, /*Annot=*/"", *STI, AndOS);
  AndOS.flush();
  EXPECT_EQ(Printed, "\tand\tr1, r2");

  Printed.clear();
  raw_string_ostream AndhOS(Printed);
  InstPrinter->printInst(&Andh, /*Address=*/0, /*Annot=*/"", *STI, AndhOS);
  AndhOS.flush();
  EXPECT_EQ(Printed, "\tandh\tr1, 1");

  Printed.clear();
  raw_string_ostream AndhCOHOS(Printed);
  InstPrinter->printInst(&AndhCOH, /*Address=*/0, /*Annot=*/"", *STI,
                         AndhCOHOS);
  AndhCOHOS.flush();
  EXPECT_EQ(Printed, "\tandh\tr1, 1, coh");

  Printed.clear();
  raw_string_ostream AndlOS(Printed);
  InstPrinter->printInst(&Andl, /*Address=*/0, /*Annot=*/"", *STI, AndlOS);
  AndlOS.flush();
  EXPECT_EQ(Printed, "\tandl\tr1, 1");

  Printed.clear();
  raw_string_ostream AndlCOHOS(Printed);
  InstPrinter->printInst(&AndlCOH, /*Address=*/0, /*Annot=*/"", *STI,
                         AndlCOHOS);
  AndlCOHOS.flush();
  EXPECT_EQ(Printed, "\tandl\tr1, 1, coh");

  Printed.clear();
  raw_string_ostream AndEqOS(Printed);
  InstPrinter->printInst(&AndEq, /*Address=*/0, /*Annot=*/"", *STI, AndEqOS);
  AndEqOS.flush();
  EXPECT_EQ(Printed, "\tandeq\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream AndAlOS(Printed);
  InstPrinter->printInst(&AndAl, /*Address=*/0, /*Annot=*/"", *STI, AndAlOS);
  AndAlOS.flush();
  EXPECT_EQ(Printed, "\tandal\tr1, r2, r3");

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
  raw_string_ostream BfextsOS(Printed);
  InstPrinter->printInst(&Bfexts, /*Address=*/0, /*Annot=*/"", *STI,
                         BfextsOS);
  BfextsOS.flush();
  EXPECT_EQ(Printed, "\tbfexts\tr1, r2, 3, 5");

  Printed.clear();
  raw_string_ostream BfextuOS(Printed);
  InstPrinter->printInst(&Bfextu, /*Address=*/0, /*Annot=*/"", *STI,
                         BfextuOS);
  BfextuOS.flush();
  EXPECT_EQ(Printed, "\tbfextu\tr1, r2, 3, 5");

  Printed.clear();
  raw_string_ostream BfinsOS(Printed);
  InstPrinter->printInst(&Bfins, /*Address=*/0, /*Annot=*/"", *STI, BfinsOS);
  BfinsOS.flush();
  EXPECT_EQ(Printed, "\tbfins\tr1, r2, 3, 5");

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
  raw_string_ostream CacheOS(Printed);
  InstPrinter->printInst(&Cache, /*Address=*/0, /*Annot=*/"", *STI, CacheOS);
  CacheOS.flush();
  EXPECT_EQ(Printed, "\tcache\tr1[-1], 1");

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
  raw_string_ostream CpBOS(Printed);
  InstPrinter->printInst(&CpB, /*Address=*/0, /*Annot=*/"", *STI, CpBOS);
  CpBOS.flush();
  EXPECT_EQ(Printed, "\tcp.b\tr1, r2");

  Printed.clear();
  raw_string_ostream CpHOS(Printed);
  InstPrinter->printInst(&CpH, /*Address=*/0, /*Annot=*/"", *STI, CpHOS);
  CpHOS.flush();
  EXPECT_EQ(Printed, "\tcp.h\tr1, r2");

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
  raw_string_ostream EorhOS(Printed);
  InstPrinter->printInst(&Eorh, /*Address=*/0, /*Annot=*/"", *STI, EorhOS);
  EorhOS.flush();
  EXPECT_EQ(Printed, "\teorh\tr1, 1");

  Printed.clear();
  raw_string_ostream EorlOS(Printed);
  InstPrinter->printInst(&Eorl, /*Address=*/0, /*Annot=*/"", *STI, EorlOS);
  EorlOS.flush();
  EXPECT_EQ(Printed, "\teorl\tr1, 1");

  Printed.clear();
  raw_string_ostream EorEqOS(Printed);
  InstPrinter->printInst(&EorEq, /*Address=*/0, /*Annot=*/"", *STI, EorEqOS);
  EorEqOS.flush();
  EXPECT_EQ(Printed, "\teoreq\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream EorAlOS(Printed);
  InstPrinter->printInst(&EorAl, /*Address=*/0, /*Annot=*/"", *STI, EorAlOS);
  EorAlOS.flush();
  EXPECT_EQ(Printed, "\teoral\tr1, r2, r3");

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
  raw_string_ostream LdWPostIncOS(Printed);
  InstPrinter->printInst(&LdWPostInc, /*Address=*/0, /*Annot=*/"", *STI,
                         LdWPostIncOS);
  LdWPostIncOS.flush();
  EXPECT_EQ(Printed, "\tld.w\tr1, r2++");

  Printed.clear();
  raw_string_ostream LdWPreDecOS(Printed);
  InstPrinter->printInst(&LdWPreDec, /*Address=*/0, /*Annot=*/"", *STI,
                         LdWPreDecOS);
  LdWPreDecOS.flush();
  EXPECT_EQ(Printed, "\tld.w\tr1, --r2");

  Printed.clear();
  raw_string_ostream LdWDisp5OS(Printed);
  InstPrinter->printInst(&LdWDisp5, /*Address=*/0, /*Annot=*/"", *STI,
                         LdWDisp5OS);
  LdWDisp5OS.flush();
  EXPECT_EQ(Printed, "\tld.w\tr1, r2[12]");

  Printed.clear();
  raw_string_ostream LdWDisp16OS(Printed);
  InstPrinter->printInst(&LdWDisp16, /*Address=*/0, /*Annot=*/"", *STI,
                         LdWDisp16OS);
  LdWDisp16OS.flush();
  EXPECT_EQ(Printed, "\tld.w\tr1, r2[-1]");

  Printed.clear();
  raw_string_ostream LdWIndexShiftOS(Printed);
  InstPrinter->printInst(&LdWIndexShift, /*Address=*/0, /*Annot=*/"", *STI,
                         LdWIndexShiftOS);
  LdWIndexShiftOS.flush();
  EXPECT_EQ(Printed, "\tld.w\tr1, r2[r3 << 2]");

  Printed.clear();
  raw_string_ostream LdWIndexPartOS(Printed);
  InstPrinter->printInst(&LdWIndexPart, /*Address=*/0, /*Annot=*/"", *STI,
                         LdWIndexPartOS);
  LdWIndexPartOS.flush();
  EXPECT_EQ(Printed, "\tld.w\tr1, r2[r3:l << 2]");

  Printed.clear();
  raw_string_ostream LdWEqDisp9OS(Printed);
  InstPrinter->printInst(&LdWEqDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         LdWEqDisp9OS);
  LdWEqDisp9OS.flush();
  EXPECT_EQ(Printed, "\tld.weq\tr1, r2[12]");

  Printed.clear();
  raw_string_ostream LdWAlDisp9OS(Printed);
  InstPrinter->printInst(&LdWAlDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         LdWAlDisp9OS);
  LdWAlDisp9OS.flush();
  EXPECT_EQ(Printed, "\tld.wal\tr1, r2[12]");

  Printed.clear();
  raw_string_ostream LdDPostIncOS(Printed);
  InstPrinter->printInst(&LdDPostInc, /*Address=*/0, /*Annot=*/"", *STI,
                         LdDPostIncOS);
  LdDPostIncOS.flush();
  EXPECT_EQ(Printed, "\tld.d\tr2, r1++");

  Printed.clear();
  raw_string_ostream LdDPreDecOS(Printed);
  InstPrinter->printInst(&LdDPreDec, /*Address=*/0, /*Annot=*/"", *STI,
                         LdDPreDecOS);
  LdDPreDecOS.flush();
  EXPECT_EQ(Printed, "\tld.d\tr2, --r1");

  Printed.clear();
  raw_string_ostream LdDRegOS(Printed);
  InstPrinter->printInst(&LdDReg, /*Address=*/0, /*Annot=*/"", *STI, LdDRegOS);
  LdDRegOS.flush();
  EXPECT_EQ(Printed, "\tld.d\tr2, r1");

  Printed.clear();
  raw_string_ostream LdDDisp16OS(Printed);
  InstPrinter->printInst(&LdDDisp16, /*Address=*/0, /*Annot=*/"", *STI,
                         LdDDisp16OS);
  LdDDisp16OS.flush();
  EXPECT_EQ(Printed, "\tld.d\tr2, r1[-1]");

  Printed.clear();
  raw_string_ostream LdDIndexShiftOS(Printed);
  InstPrinter->printInst(&LdDIndexShift, /*Address=*/0, /*Annot=*/"", *STI,
                         LdDIndexShiftOS);
  LdDIndexShiftOS.flush();
  EXPECT_EQ(Printed, "\tld.d\tr2, r1[r3 << 3]");

  Printed.clear();
  raw_string_ostream LddpcOS(Printed);
  InstPrinter->printInst(&Lddpc, /*Address=*/0, /*Annot=*/"", *STI, LddpcOS);
  LddpcOS.flush();
  EXPECT_EQ(Printed, "\tlddpc\tr2, pc[12]");

  Printed.clear();
  raw_string_ostream LddspOS(Printed);
  InstPrinter->printInst(&Lddsp, /*Address=*/0, /*Annot=*/"", *STI, LddspOS);
  LddspOS.flush();
  EXPECT_EQ(Printed, "\tlddsp\tr2, sp[12]");

  Printed.clear();
  raw_string_ostream LdinsBOS(Printed);
  InstPrinter->printInst(&LdinsB, /*Address=*/0, /*Annot=*/"", *STI,
                         LdinsBOS);
  LdinsBOS.flush();
  EXPECT_EQ(Printed, "\tldins.b\tr1:u, r2[-1]");

  Printed.clear();
  raw_string_ostream LdinsHOS(Printed);
  InstPrinter->printInst(&LdinsH, /*Address=*/0, /*Annot=*/"", *STI,
                         LdinsHOS);
  LdinsHOS.flush();
  EXPECT_EQ(Printed, "\tldins.h\tr1:t, r2[-2]");

  Printed.clear();
  raw_string_ostream LdSBDisp16OS(Printed);
  InstPrinter->printInst(&LdSBDisp16, /*Address=*/0, /*Annot=*/"", *STI,
                         LdSBDisp16OS);
  LdSBDisp16OS.flush();
  EXPECT_EQ(Printed, "\tld.sb\tr1, r2[-1]");

  Printed.clear();
  raw_string_ostream LdSBIndexShiftOS(Printed);
  InstPrinter->printInst(&LdSBIndexShift, /*Address=*/0, /*Annot=*/"", *STI,
                         LdSBIndexShiftOS);
  LdSBIndexShiftOS.flush();
  EXPECT_EQ(Printed, "\tld.sb\tr1, r2[r3 << 2]");

  Printed.clear();
  raw_string_ostream LdSBEqDisp9OS(Printed);
  InstPrinter->printInst(&LdSBEqDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         LdSBEqDisp9OS);
  LdSBEqDisp9OS.flush();
  EXPECT_EQ(Printed, "\tld.sbeq\tr1, r2[3]");

  Printed.clear();
  raw_string_ostream LdSBAlDisp9OS(Printed);
  InstPrinter->printInst(&LdSBAlDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         LdSBAlDisp9OS);
  LdSBAlDisp9OS.flush();
  EXPECT_EQ(Printed, "\tld.sbal\tr1, r2[3]");

  Printed.clear();
  raw_string_ostream LdUBPostIncOS(Printed);
  InstPrinter->printInst(&LdUBPostInc, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUBPostIncOS);
  LdUBPostIncOS.flush();
  EXPECT_EQ(Printed, "\tld.ub\tr1, r2++");

  Printed.clear();
  raw_string_ostream LdUBPreDecOS(Printed);
  InstPrinter->printInst(&LdUBPreDec, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUBPreDecOS);
  LdUBPreDecOS.flush();
  EXPECT_EQ(Printed, "\tld.ub\tr1, --r2");

  Printed.clear();
  raw_string_ostream LdUBDisp3OS(Printed);
  InstPrinter->printInst(&LdUBDisp3, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUBDisp3OS);
  LdUBDisp3OS.flush();
  EXPECT_EQ(Printed, "\tld.ub\tr1, r2[3]");

  Printed.clear();
  raw_string_ostream LdUBDisp16OS(Printed);
  InstPrinter->printInst(&LdUBDisp16, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUBDisp16OS);
  LdUBDisp16OS.flush();
  EXPECT_EQ(Printed, "\tld.ub\tr1, r2[-1]");

  Printed.clear();
  raw_string_ostream LdUBIndexShiftOS(Printed);
  InstPrinter->printInst(&LdUBIndexShift, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUBIndexShiftOS);
  LdUBIndexShiftOS.flush();
  EXPECT_EQ(Printed, "\tld.ub\tr1, r2[r3 << 2]");

  Printed.clear();
  raw_string_ostream LdUBEqDisp9OS(Printed);
  InstPrinter->printInst(&LdUBEqDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUBEqDisp9OS);
  LdUBEqDisp9OS.flush();
  EXPECT_EQ(Printed, "\tld.ubeq\tr1, r2[3]");

  Printed.clear();
  raw_string_ostream LdUBAlDisp9OS(Printed);
  InstPrinter->printInst(&LdUBAlDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUBAlDisp9OS);
  LdUBAlDisp9OS.flush();
  EXPECT_EQ(Printed, "\tld.ubal\tr1, r2[3]");

  Printed.clear();
  raw_string_ostream LdSHPostIncOS(Printed);
  InstPrinter->printInst(&LdSHPostInc, /*Address=*/0, /*Annot=*/"", *STI,
                         LdSHPostIncOS);
  LdSHPostIncOS.flush();
  EXPECT_EQ(Printed, "\tld.sh\tr1, r2++");

  Printed.clear();
  raw_string_ostream LdSHPreDecOS(Printed);
  InstPrinter->printInst(&LdSHPreDec, /*Address=*/0, /*Annot=*/"", *STI,
                         LdSHPreDecOS);
  LdSHPreDecOS.flush();
  EXPECT_EQ(Printed, "\tld.sh\tr1, --r2");

  Printed.clear();
  raw_string_ostream LdSHDisp3OS(Printed);
  InstPrinter->printInst(&LdSHDisp3, /*Address=*/0, /*Annot=*/"", *STI,
                         LdSHDisp3OS);
  LdSHDisp3OS.flush();
  EXPECT_EQ(Printed, "\tld.sh\tr1, r2[6]");

  Printed.clear();
  raw_string_ostream LdSHDisp16OS(Printed);
  InstPrinter->printInst(&LdSHDisp16, /*Address=*/0, /*Annot=*/"", *STI,
                         LdSHDisp16OS);
  LdSHDisp16OS.flush();
  EXPECT_EQ(Printed, "\tld.sh\tr1, r2[-1]");

  Printed.clear();
  raw_string_ostream LdSHIndexShiftOS(Printed);
  InstPrinter->printInst(&LdSHIndexShift, /*Address=*/0, /*Annot=*/"", *STI,
                         LdSHIndexShiftOS);
  LdSHIndexShiftOS.flush();
  EXPECT_EQ(Printed, "\tld.sh\tr1, r2[r3 << 2]");

  Printed.clear();
  raw_string_ostream LdSHEqDisp9OS(Printed);
  InstPrinter->printInst(&LdSHEqDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         LdSHEqDisp9OS);
  LdSHEqDisp9OS.flush();
  EXPECT_EQ(Printed, "\tld.sheq\tr1, r2[6]");

  Printed.clear();
  raw_string_ostream LdSHAlDisp9OS(Printed);
  InstPrinter->printInst(&LdSHAlDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         LdSHAlDisp9OS);
  LdSHAlDisp9OS.flush();
  EXPECT_EQ(Printed, "\tld.shal\tr1, r2[6]");

  Printed.clear();
  raw_string_ostream LdUHPostIncOS(Printed);
  InstPrinter->printInst(&LdUHPostInc, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUHPostIncOS);
  LdUHPostIncOS.flush();
  EXPECT_EQ(Printed, "\tld.uh\tr1, r2++");

  Printed.clear();
  raw_string_ostream LdUHPreDecOS(Printed);
  InstPrinter->printInst(&LdUHPreDec, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUHPreDecOS);
  LdUHPreDecOS.flush();
  EXPECT_EQ(Printed, "\tld.uh\tr1, --r2");

  Printed.clear();
  raw_string_ostream LdUHDisp3OS(Printed);
  InstPrinter->printInst(&LdUHDisp3, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUHDisp3OS);
  LdUHDisp3OS.flush();
  EXPECT_EQ(Printed, "\tld.uh\tr1, r2[6]");

  Printed.clear();
  raw_string_ostream LdUHDisp16OS(Printed);
  InstPrinter->printInst(&LdUHDisp16, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUHDisp16OS);
  LdUHDisp16OS.flush();
  EXPECT_EQ(Printed, "\tld.uh\tr1, r2[-1]");

  Printed.clear();
  raw_string_ostream LdUHIndexShiftOS(Printed);
  InstPrinter->printInst(&LdUHIndexShift, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUHIndexShiftOS);
  LdUHIndexShiftOS.flush();
  EXPECT_EQ(Printed, "\tld.uh\tr1, r2[r3 << 2]");

  Printed.clear();
  raw_string_ostream LdUHEqDisp9OS(Printed);
  InstPrinter->printInst(&LdUHEqDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUHEqDisp9OS);
  LdUHEqDisp9OS.flush();
  EXPECT_EQ(Printed, "\tld.uheq\tr1, r2[6]");

  Printed.clear();
  raw_string_ostream LdUHAlDisp9OS(Printed);
  InstPrinter->printInst(&LdUHAlDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         LdUHAlDisp9OS);
  LdUHAlDisp9OS.flush();
  EXPECT_EQ(Printed, "\tld.uhal\tr1, r2[6]");

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
  raw_string_ostream DivsOS(Printed);
  InstPrinter->printInst(&Divs, /*Address=*/0, /*Annot=*/"", *STI, DivsOS);
  DivsOS.flush();
  EXPECT_EQ(Printed, "\tdivs\tr2, r3, r4");

  Printed.clear();
  raw_string_ostream DivuOS(Printed);
  InstPrinter->printInst(&Divu, /*Address=*/0, /*Annot=*/"", *STI, DivuOS);
  DivuOS.flush();
  EXPECT_EQ(Printed, "\tdivu\tr2, r3, r4");

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
  raw_string_ostream MemcOS(Printed);
  InstPrinter->printInst(&Memc, /*Address=*/0, /*Annot=*/"", *STI, MemcOS);
  MemcOS.flush();
  EXPECT_EQ(Printed, "\tmemc\t4, 1");

  Printed.clear();
  raw_string_ostream MemsOS(Printed);
  InstPrinter->printInst(&Mems, /*Address=*/0, /*Annot=*/"", *STI, MemsOS);
  MemsOS.flush();
  EXPECT_EQ(Printed, "\tmems\t4, 1");

  Printed.clear();
  raw_string_ostream MemtOS(Printed);
  InstPrinter->printInst(&Memt, /*Address=*/0, /*Annot=*/"", *STI, MemtOS);
  MemtOS.flush();
  EXPECT_EQ(Printed, "\tmemt\t4, 1");

  Printed.clear();
  raw_string_ostream OrOS(Printed);
  InstPrinter->printInst(&Or, /*Address=*/0, /*Annot=*/"", *STI, OrOS);
  OrOS.flush();
  EXPECT_EQ(Printed, "\tor\tr1, r2");

  Printed.clear();
  raw_string_ostream OrhOS(Printed);
  InstPrinter->printInst(&Orh, /*Address=*/0, /*Annot=*/"", *STI, OrhOS);
  OrhOS.flush();
  EXPECT_EQ(Printed, "\torh\tr1, 1");

  Printed.clear();
  raw_string_ostream OrlOS(Printed);
  InstPrinter->printInst(&Orl, /*Address=*/0, /*Annot=*/"", *STI, OrlOS);
  OrlOS.flush();
  EXPECT_EQ(Printed, "\torl\tr1, 1");

  Printed.clear();
  raw_string_ostream PrefOS(Printed);
  InstPrinter->printInst(&Pref, /*Address=*/0, /*Annot=*/"", *STI, PrefOS);
  PrefOS.flush();
  EXPECT_EQ(Printed, "\tpref\tr1[-1]");

  Printed.clear();
  raw_string_ostream PabsSBOS(Printed);
  InstPrinter->printInst(&PabsSB, /*Address=*/0, /*Annot=*/"", *STI, PabsSBOS);
  PabsSBOS.flush();
  EXPECT_EQ(Printed, "\tpabs.sb\tr1, r2");

  Printed.clear();
  raw_string_ostream PabsSHOS(Printed);
  InstPrinter->printInst(&PabsSH, /*Address=*/0, /*Annot=*/"", *STI, PabsSHOS);
  PabsSHOS.flush();
  EXPECT_EQ(Printed, "\tpabs.sh\tr1, r2");

  Printed.clear();
  raw_string_ostream PackshUBOS(Printed);
  InstPrinter->printInst(&PackshUB, /*Address=*/0, /*Annot=*/"", *STI,
                         PackshUBOS);
  PackshUBOS.flush();
  EXPECT_EQ(Printed, "\tpacksh.ub\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PackshSBOS(Printed);
  InstPrinter->printInst(&PackshSB, /*Address=*/0, /*Annot=*/"", *STI,
                         PackshSBOS);
  PackshSBOS.flush();
  EXPECT_EQ(Printed, "\tpacksh.sb\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PackwSHOS(Printed);
  InstPrinter->printInst(&PackwSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PackwSHOS);
  PackwSHOS.flush();
  EXPECT_EQ(Printed, "\tpackw.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddBOS(Printed);
  InstPrinter->printInst(&PaddB, /*Address=*/0, /*Annot=*/"", *STI, PaddBOS);
  PaddBOS.flush();
  EXPECT_EQ(Printed, "\tpadd.b\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddHOS(Printed);
  InstPrinter->printInst(&PaddH, /*Address=*/0, /*Annot=*/"", *STI, PaddHOS);
  PaddHOS.flush();
  EXPECT_EQ(Printed, "\tpadd.h\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddhUBOS(Printed);
  InstPrinter->printInst(&PaddhUB, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddhUBOS);
  PaddhUBOS.flush();
  EXPECT_EQ(Printed, "\tpaddh.ub\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddhSHOS(Printed);
  InstPrinter->printInst(&PaddhSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddhSHOS);
  PaddhSHOS.flush();
  EXPECT_EQ(Printed, "\tpaddh.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddsUBOS(Printed);
  InstPrinter->printInst(&PaddsUB, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddsUBOS);
  PaddsUBOS.flush();
  EXPECT_EQ(Printed, "\tpadds.ub\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddsSBOS(Printed);
  InstPrinter->printInst(&PaddsSB, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddsSBOS);
  PaddsSBOS.flush();
  EXPECT_EQ(Printed, "\tpadds.sb\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddsUHOS(Printed);
  InstPrinter->printInst(&PaddsUH, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddsUHOS);
  PaddsUHOS.flush();
  EXPECT_EQ(Printed, "\tpadds.uh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddsSHOS(Printed);
  InstPrinter->printInst(&PaddsSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddsSHOS);
  PaddsSHOS.flush();
  EXPECT_EQ(Printed, "\tpadds.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddsubHOS(Printed);
  InstPrinter->printInst(&PaddsubH, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddsubHOS);
  PaddsubHOS.flush();
  EXPECT_EQ(Printed, "\tpaddsub.h\tr1, r2:t, r3:b");

  Printed.clear();
  raw_string_ostream PaddsubhSHOS(Printed);
  InstPrinter->printInst(&PaddsubhSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddsubhSHOS);
  PaddsubhSHOS.flush();
  EXPECT_EQ(Printed, "\tpaddsubh.sh\tr1, r2:t, r3:b");

  Printed.clear();
  raw_string_ostream PaddsubsUHOS(Printed);
  InstPrinter->printInst(&PaddsubsUH, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddsubsUHOS);
  PaddsubsUHOS.flush();
  EXPECT_EQ(Printed, "\tpaddsubs.uh\tr1, r2:t, r3:b");

  Printed.clear();
  raw_string_ostream PaddsubsSHOS(Printed);
  InstPrinter->printInst(&PaddsubsSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddsubsSHOS);
  PaddsubsSHOS.flush();
  EXPECT_EQ(Printed, "\tpaddsubs.sh\tr1, r2:t, r3:b");

  Printed.clear();
  raw_string_ostream PaddxHOS(Printed);
  InstPrinter->printInst(&PaddxH, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddxHOS);
  PaddxHOS.flush();
  EXPECT_EQ(Printed, "\tpaddx.h\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddxhSHOS(Printed);
  InstPrinter->printInst(&PaddxhSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddxhSHOS);
  PaddxhSHOS.flush();
  EXPECT_EQ(Printed, "\tpaddxh.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddxsUHOS(Printed);
  InstPrinter->printInst(&PaddxsUH, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddxsUHOS);
  PaddxsUHOS.flush();
  EXPECT_EQ(Printed, "\tpaddxs.uh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PaddxsSHOS(Printed);
  InstPrinter->printInst(&PaddxsSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PaddxsSHOS);
  PaddxsSHOS.flush();
  EXPECT_EQ(Printed, "\tpaddxs.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PasrBOS(Printed);
  InstPrinter->printInst(&PasrB, /*Address=*/0, /*Annot=*/"", *STI, PasrBOS);
  PasrBOS.flush();
  EXPECT_EQ(Printed, "\tpasr.b\tr1, r2, 3");

  Printed.clear();
  raw_string_ostream PasrHOS(Printed);
  InstPrinter->printInst(&PasrH, /*Address=*/0, /*Annot=*/"", *STI, PasrHOS);
  PasrHOS.flush();
  EXPECT_EQ(Printed, "\tpasr.h\tr1, r2, 15");

  Printed.clear();
  raw_string_ostream PlslBOS(Printed);
  InstPrinter->printInst(&PlslB, /*Address=*/0, /*Annot=*/"", *STI, PlslBOS);
  PlslBOS.flush();
  EXPECT_EQ(Printed, "\tplsl.b\tr1, r2, 3");

  Printed.clear();
  raw_string_ostream PlslHOS(Printed);
  InstPrinter->printInst(&PlslH, /*Address=*/0, /*Annot=*/"", *STI, PlslHOS);
  PlslHOS.flush();
  EXPECT_EQ(Printed, "\tplsl.h\tr1, r2, 15");

  Printed.clear();
  raw_string_ostream PlsrBOS(Printed);
  InstPrinter->printInst(&PlsrB, /*Address=*/0, /*Annot=*/"", *STI, PlsrBOS);
  PlsrBOS.flush();
  EXPECT_EQ(Printed, "\tplsr.b\tr1, r2, 3");

  Printed.clear();
  raw_string_ostream PlsrHOS(Printed);
  InstPrinter->printInst(&PlsrH, /*Address=*/0, /*Annot=*/"", *STI, PlsrHOS);
  PlsrHOS.flush();
  EXPECT_EQ(Printed, "\tplsr.h\tr1, r2, 15");

  Printed.clear();
  raw_string_ostream PavgUBOS(Printed);
  InstPrinter->printInst(&PavgUB, /*Address=*/0, /*Annot=*/"", *STI,
                         PavgUBOS);
  PavgUBOS.flush();
  EXPECT_EQ(Printed, "\tpavg.ub\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PavgSHOS(Printed);
  InstPrinter->printInst(&PavgSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PavgSHOS);
  PavgSHOS.flush();
  EXPECT_EQ(Printed, "\tpavg.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PmaxUBOS(Printed);
  InstPrinter->printInst(&PmaxUB, /*Address=*/0, /*Annot=*/"", *STI,
                         PmaxUBOS);
  PmaxUBOS.flush();
  EXPECT_EQ(Printed, "\tpmax.ub\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PmaxSHOS(Printed);
  InstPrinter->printInst(&PmaxSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PmaxSHOS);
  PmaxSHOS.flush();
  EXPECT_EQ(Printed, "\tpmax.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PminUBOS(Printed);
  InstPrinter->printInst(&PminUB, /*Address=*/0, /*Annot=*/"", *STI,
                         PminUBOS);
  PminUBOS.flush();
  EXPECT_EQ(Printed, "\tpmin.ub\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PminSHOS(Printed);
  InstPrinter->printInst(&PminSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PminSHOS);
  PminSHOS.flush();
  EXPECT_EQ(Printed, "\tpmin.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsadOS(Printed);
  InstPrinter->printInst(&Psad, /*Address=*/0, /*Annot=*/"", *STI, PsadOS);
  PsadOS.flush();
  EXPECT_EQ(Printed, "\tpsad\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubBOS(Printed);
  InstPrinter->printInst(&PsubB, /*Address=*/0, /*Annot=*/"", *STI, PsubBOS);
  PsubBOS.flush();
  EXPECT_EQ(Printed, "\tpsub.b\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubHOS(Printed);
  InstPrinter->printInst(&PsubH, /*Address=*/0, /*Annot=*/"", *STI, PsubHOS);
  PsubHOS.flush();
  EXPECT_EQ(Printed, "\tpsub.h\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubaddHOS(Printed);
  InstPrinter->printInst(&PsubaddH, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubaddHOS);
  PsubaddHOS.flush();
  EXPECT_EQ(Printed, "\tpsubadd.h\tr1, r2:t, r3:b");

  Printed.clear();
  raw_string_ostream PsubaddhSHOS(Printed);
  InstPrinter->printInst(&PsubaddhSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubaddhSHOS);
  PsubaddhSHOS.flush();
  EXPECT_EQ(Printed, "\tpsubaddh.sh\tr1, r2:t, r3:b");

  Printed.clear();
  raw_string_ostream PsubaddsUHOS(Printed);
  InstPrinter->printInst(&PsubaddsUH, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubaddsUHOS);
  PsubaddsUHOS.flush();
  EXPECT_EQ(Printed, "\tpsubadds.uh\tr1, r2:t, r3:b");

  Printed.clear();
  raw_string_ostream PsubaddsSHOS(Printed);
  InstPrinter->printInst(&PsubaddsSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubaddsSHOS);
  PsubaddsSHOS.flush();
  EXPECT_EQ(Printed, "\tpsubadds.sh\tr1, r2:t, r3:b");

  Printed.clear();
  raw_string_ostream PsubhUBOS(Printed);
  InstPrinter->printInst(&PsubhUB, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubhUBOS);
  PsubhUBOS.flush();
  EXPECT_EQ(Printed, "\tpsubh.ub\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubhSHOS(Printed);
  InstPrinter->printInst(&PsubhSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubhSHOS);
  PsubhSHOS.flush();
  EXPECT_EQ(Printed, "\tpsubh.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubsUBOS(Printed);
  InstPrinter->printInst(&PsubsUB, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubsUBOS);
  PsubsUBOS.flush();
  EXPECT_EQ(Printed, "\tpsubs.ub\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubsSBOS(Printed);
  InstPrinter->printInst(&PsubsSB, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubsSBOS);
  PsubsSBOS.flush();
  EXPECT_EQ(Printed, "\tpsubs.sb\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubsUHOS(Printed);
  InstPrinter->printInst(&PsubsUH, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubsUHOS);
  PsubsUHOS.flush();
  EXPECT_EQ(Printed, "\tpsubs.uh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubsSHOS(Printed);
  InstPrinter->printInst(&PsubsSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubsSHOS);
  PsubsSHOS.flush();
  EXPECT_EQ(Printed, "\tpsubs.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubxHOS(Printed);
  InstPrinter->printInst(&PsubxH, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubxHOS);
  PsubxHOS.flush();
  EXPECT_EQ(Printed, "\tpsubx.h\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubxhSHOS(Printed);
  InstPrinter->printInst(&PsubxhSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubxhSHOS);
  PsubxhSHOS.flush();
  EXPECT_EQ(Printed, "\tpsubxh.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubxsUHOS(Printed);
  InstPrinter->printInst(&PsubxsUH, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubxsUHOS);
  PsubxsUHOS.flush();
  EXPECT_EQ(Printed, "\tpsubxs.uh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PsubxsSHOS(Printed);
  InstPrinter->printInst(&PsubxsSH, /*Address=*/0, /*Annot=*/"", *STI,
                         PsubxsSHOS);
  PsubxsSHOS.flush();
  EXPECT_EQ(Printed, "\tpsubxs.sh\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream PunpckubHOS(Printed);
  InstPrinter->printInst(&PunpckubH, /*Address=*/0, /*Annot=*/"", *STI,
                         PunpckubHOS);
  PunpckubHOS.flush();
  EXPECT_EQ(Printed, "\tpunpckub.h\tr1, r2:b");

  Printed.clear();
  raw_string_ostream PunpcksbHOS(Printed);
  InstPrinter->printInst(&PunpcksbH, /*Address=*/0, /*Annot=*/"", *STI,
                         PunpcksbHOS);
  PunpcksbHOS.flush();
  EXPECT_EQ(Printed, "\tpunpcksb.h\tr1, r2:t");

  Printed.clear();
  raw_string_ostream OrEqOS(Printed);
  InstPrinter->printInst(&OrEq, /*Address=*/0, /*Annot=*/"", *STI, OrEqOS);
  OrEqOS.flush();
  EXPECT_EQ(Printed, "\toreq\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream OrAlOS(Printed);
  InstPrinter->printInst(&OrAl, /*Address=*/0, /*Annot=*/"", *STI, OrAlOS);
  OrAlOS.flush();
  EXPECT_EQ(Printed, "\toral\tr1, r2, r3");

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
  raw_string_ostream RsubEqOS(Printed);
  InstPrinter->printInst(&RsubEq, /*Address=*/0, /*Annot=*/"", *STI,
                         RsubEqOS);
  RsubEqOS.flush();
  EXPECT_EQ(Printed, "\trsubeq\tr1, -1");

  Printed.clear();
  raw_string_ostream RsubAlOS(Printed);
  InstPrinter->printInst(&RsubAl, /*Address=*/0, /*Annot=*/"", *STI,
                         RsubAlOS);
  RsubAlOS.flush();
  EXPECT_EQ(Printed, "\trsubal\tr1, -1");

  Printed.clear();
  raw_string_ostream SataddWOS(Printed);
  InstPrinter->printInst(&SataddW, /*Address=*/0, /*Annot=*/"", *STI,
                         SataddWOS);
  SataddWOS.flush();
  EXPECT_EQ(Printed, "\tsatadd.w\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream SatsubHOS(Printed);
  InstPrinter->printInst(&SatsubH, /*Address=*/0, /*Annot=*/"", *STI,
                         SatsubHOS);
  SatsubHOS.flush();
  EXPECT_EQ(Printed, "\tsatsub.h\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream SatsubWOS(Printed);
  InstPrinter->printInst(&SatsubW, /*Address=*/0, /*Annot=*/"", *STI,
                         SatsubWOS);
  SatsubWOS.flush();
  EXPECT_EQ(Printed, "\tsatsub.w\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream SatsubWImmOS(Printed);
  InstPrinter->printInst(&SatsubWImm, /*Address=*/0, /*Annot=*/"", *STI,
                         SatsubWImmOS);
  SatsubWImmOS.flush();
  EXPECT_EQ(Printed, "\tsatsub.w\tr1, r2, -1");

  Printed.clear();
  raw_string_ostream SatrndsOS(Printed);
  InstPrinter->printInst(&Satrnds, /*Address=*/0, /*Annot=*/"", *STI,
                         SatrndsOS);
  SatrndsOS.flush();
  EXPECT_EQ(Printed, "\tsatrnds\tr1 >> 2, 3");

  Printed.clear();
  raw_string_ostream SatrnduOS(Printed);
  InstPrinter->printInst(&Satrndu, /*Address=*/0, /*Annot=*/"", *STI,
                         SatrnduOS);
  SatrnduOS.flush();
  EXPECT_EQ(Printed, "\tsatrndu\tr1 >> 2, 3");

  Printed.clear();
  raw_string_ostream SatsOS(Printed);
  InstPrinter->printInst(&Sats, /*Address=*/0, /*Annot=*/"", *STI, SatsOS);
  SatsOS.flush();
  EXPECT_EQ(Printed, "\tsats\tr1 >> 2, 3");

  Printed.clear();
  raw_string_ostream SatuOS(Printed);
  InstPrinter->printInst(&Satu, /*Address=*/0, /*Annot=*/"", *STI, SatuOS);
  SatuOS.flush();
  EXPECT_EQ(Printed, "\tsatu\tr1 >> 2, 3");

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
  raw_string_ostream StBPostIncOS(Printed);
  InstPrinter->printInst(&StBPostInc, /*Address=*/0, /*Annot=*/"", *STI,
                         StBPostIncOS);
  StBPostIncOS.flush();
  EXPECT_EQ(Printed, "\tst.b\tr1++, r2");

  Printed.clear();
  raw_string_ostream StBPreDecOS(Printed);
  InstPrinter->printInst(&StBPreDec, /*Address=*/0, /*Annot=*/"", *STI,
                         StBPreDecOS);
  StBPreDecOS.flush();
  EXPECT_EQ(Printed, "\tst.b\t--r1, r2");

  Printed.clear();
  raw_string_ostream StBDisp3OS(Printed);
  InstPrinter->printInst(&StBDisp3, /*Address=*/0, /*Annot=*/"", *STI,
                         StBDisp3OS);
  StBDisp3OS.flush();
  EXPECT_EQ(Printed, "\tst.b\tr1[3], r2");

  Printed.clear();
  raw_string_ostream StBDisp16OS(Printed);
  InstPrinter->printInst(&StBDisp16, /*Address=*/0, /*Annot=*/"", *STI,
                         StBDisp16OS);
  StBDisp16OS.flush();
  EXPECT_EQ(Printed, "\tst.b\tr1[-1], r2");

  Printed.clear();
  raw_string_ostream StBIndexShiftOS(Printed);
  InstPrinter->printInst(&StBIndexShift, /*Address=*/0, /*Annot=*/"", *STI,
                         StBIndexShiftOS);
  StBIndexShiftOS.flush();
  EXPECT_EQ(Printed, "\tst.b\tr1[r2 << 3], r4");

  Printed.clear();
  raw_string_ostream StBEqDisp9OS(Printed);
  InstPrinter->printInst(&StBEqDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         StBEqDisp9OS);
  StBEqDisp9OS.flush();
  EXPECT_EQ(Printed, "\tst.beq\tr1[3], r2");

  Printed.clear();
  raw_string_ostream StBAlDisp9OS(Printed);
  InstPrinter->printInst(&StBAlDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         StBAlDisp9OS);
  StBAlDisp9OS.flush();
  EXPECT_EQ(Printed, "\tst.bal\tr1[3], r2");

  Printed.clear();
  raw_string_ostream StDPostIncOS(Printed);
  InstPrinter->printInst(&StDPostInc, /*Address=*/0, /*Annot=*/"", *STI,
                         StDPostIncOS);
  StDPostIncOS.flush();
  EXPECT_EQ(Printed, "\tst.d\tr1++, r2");

  Printed.clear();
  raw_string_ostream StDPreDecOS(Printed);
  InstPrinter->printInst(&StDPreDec, /*Address=*/0, /*Annot=*/"", *STI,
                         StDPreDecOS);
  StDPreDecOS.flush();
  EXPECT_EQ(Printed, "\tst.d\t--r1, r2");

  Printed.clear();
  raw_string_ostream StDRegOS(Printed);
  InstPrinter->printInst(&StDReg, /*Address=*/0, /*Annot=*/"", *STI, StDRegOS);
  StDRegOS.flush();
  EXPECT_EQ(Printed, "\tst.d\tr1, r2");

  Printed.clear();
  raw_string_ostream StDDisp16OS(Printed);
  InstPrinter->printInst(&StDDisp16, /*Address=*/0, /*Annot=*/"", *STI,
                         StDDisp16OS);
  StDDisp16OS.flush();
  EXPECT_EQ(Printed, "\tst.d\tr1[-1], r2");

  Printed.clear();
  raw_string_ostream StDIndexShiftOS(Printed);
  InstPrinter->printInst(&StDIndexShift, /*Address=*/0, /*Annot=*/"", *STI,
                         StDIndexShiftOS);
  StDIndexShiftOS.flush();
  EXPECT_EQ(Printed, "\tst.d\tr1[r2 << 3], r4");

  Printed.clear();
  raw_string_ostream StHPostIncOS(Printed);
  InstPrinter->printInst(&StHPostInc, /*Address=*/0, /*Annot=*/"", *STI,
                         StHPostIncOS);
  StHPostIncOS.flush();
  EXPECT_EQ(Printed, "\tst.h\tr1++, r2");

  Printed.clear();
  raw_string_ostream StHPreDecOS(Printed);
  InstPrinter->printInst(&StHPreDec, /*Address=*/0, /*Annot=*/"", *STI,
                         StHPreDecOS);
  StHPreDecOS.flush();
  EXPECT_EQ(Printed, "\tst.h\t--r1, r2");

  Printed.clear();
  raw_string_ostream StHDisp3OS(Printed);
  InstPrinter->printInst(&StHDisp3, /*Address=*/0, /*Annot=*/"", *STI,
                         StHDisp3OS);
  StHDisp3OS.flush();
  EXPECT_EQ(Printed, "\tst.h\tr1[6], r2");

  Printed.clear();
  raw_string_ostream StHDisp16OS(Printed);
  InstPrinter->printInst(&StHDisp16, /*Address=*/0, /*Annot=*/"", *STI,
                         StHDisp16OS);
  StHDisp16OS.flush();
  EXPECT_EQ(Printed, "\tst.h\tr1[-1], r2");

  Printed.clear();
  raw_string_ostream StHIndexShiftOS(Printed);
  InstPrinter->printInst(&StHIndexShift, /*Address=*/0, /*Annot=*/"", *STI,
                         StHIndexShiftOS);
  StHIndexShiftOS.flush();
  EXPECT_EQ(Printed, "\tst.h\tr1[r2 << 3], r4");

  Printed.clear();
  raw_string_ostream StHEqDisp9OS(Printed);
  InstPrinter->printInst(&StHEqDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         StHEqDisp9OS);
  StHEqDisp9OS.flush();
  EXPECT_EQ(Printed, "\tst.heq\tr1[6], r2");

  Printed.clear();
  raw_string_ostream StHAlDisp9OS(Printed);
  InstPrinter->printInst(&StHAlDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         StHAlDisp9OS);
  StHAlDisp9OS.flush();
  EXPECT_EQ(Printed, "\tst.hal\tr1[6], r2");

  Printed.clear();
  raw_string_ostream StWPostIncOS(Printed);
  InstPrinter->printInst(&StWPostInc, /*Address=*/0, /*Annot=*/"", *STI,
                         StWPostIncOS);
  StWPostIncOS.flush();
  EXPECT_EQ(Printed, "\tst.w\tr1++, r2");

  Printed.clear();
  raw_string_ostream StWPreDecOS(Printed);
  InstPrinter->printInst(&StWPreDec, /*Address=*/0, /*Annot=*/"", *STI,
                         StWPreDecOS);
  StWPreDecOS.flush();
  EXPECT_EQ(Printed, "\tst.w\t--r1, r2");

  Printed.clear();
  raw_string_ostream StWDisp4OS(Printed);
  InstPrinter->printInst(&StWDisp4, /*Address=*/0, /*Annot=*/"", *STI,
                         StWDisp4OS);
  StWDisp4OS.flush();
  EXPECT_EQ(Printed, "\tst.w\tr1[12], r2");

  Printed.clear();
  raw_string_ostream StWDisp16OS(Printed);
  InstPrinter->printInst(&StWDisp16, /*Address=*/0, /*Annot=*/"", *STI,
                         StWDisp16OS);
  StWDisp16OS.flush();
  EXPECT_EQ(Printed, "\tst.w\tr1[-1], r2");

  Printed.clear();
  raw_string_ostream StWIndexShiftOS(Printed);
  InstPrinter->printInst(&StWIndexShift, /*Address=*/0, /*Annot=*/"", *STI,
                         StWIndexShiftOS);
  StWIndexShiftOS.flush();
  EXPECT_EQ(Printed, "\tst.w\tr1[r2 << 3], r4");

  Printed.clear();
  raw_string_ostream StWEqDisp9OS(Printed);
  InstPrinter->printInst(&StWEqDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         StWEqDisp9OS);
  StWEqDisp9OS.flush();
  EXPECT_EQ(Printed, "\tst.weq\tr1[12], r2");

  Printed.clear();
  raw_string_ostream StWAlDisp9OS(Printed);
  InstPrinter->printInst(&StWAlDisp9, /*Address=*/0, /*Annot=*/"", *STI,
                         StWAlDisp9OS);
  StWAlDisp9OS.flush();
  EXPECT_EQ(Printed, "\tst.wal\tr1[12], r2");

  Printed.clear();
  raw_string_ostream StcondOS(Printed);
  InstPrinter->printInst(&Stcond, /*Address=*/0, /*Annot=*/"", *STI,
                         StcondOS);
  StcondOS.flush();
  EXPECT_EQ(Printed, "\tstcond\tr1[-1], r2");

  Printed.clear();
  raw_string_ostream StdspOS(Printed);
  InstPrinter->printInst(&Stdsp, /*Address=*/0, /*Annot=*/"", *STI, StdspOS);
  StdspOS.flush();
  EXPECT_EQ(Printed, "\tstdsp\tsp[12], r2");

  Printed.clear();
  raw_string_ostream SthhWDisp8OS(Printed);
  InstPrinter->printInst(&SthhWDisp8, /*Address=*/0, /*Annot=*/"", *STI,
                         SthhWDisp8OS);
  SthhWDisp8OS.flush();
  EXPECT_EQ(Printed, "\tsthh.w\tr1[12], r2:t, r3:b");

  Printed.clear();
  raw_string_ostream SthhWIndexShiftOS(Printed);
  InstPrinter->printInst(&SthhWIndexShift, /*Address=*/0, /*Annot=*/"",
                         *STI, SthhWIndexShiftOS);
  SthhWIndexShiftOS.flush();
  EXPECT_EQ(Printed, "\tsthh.w\tr1[r4 << 3], r2:t, r3:b");

  Printed.clear();
  raw_string_ostream StswpHDisp12OS(Printed);
  InstPrinter->printInst(&StswpHDisp12, /*Address=*/0, /*Annot=*/"", *STI,
                         StswpHDisp12OS);
  StswpHDisp12OS.flush();
  EXPECT_EQ(Printed, "\tstswp.h\tr1[-2], r2");

  Printed.clear();
  raw_string_ostream SubhhWOS(Printed);
  InstPrinter->printInst(&SubhhW, /*Address=*/0, /*Annot=*/"", *STI,
                         SubhhWOS);
  SubhhWOS.flush();
  EXPECT_EQ(Printed, "\tsubhh.w\tr1, r2:t, r3:b");

  Printed.clear();
  raw_string_ostream StswpWDisp12OS(Printed);
  InstPrinter->printInst(&StswpWDisp12, /*Address=*/0, /*Annot=*/"", *STI,
                         StswpWDisp12OS);
  StswpWDisp12OS.flush();
  EXPECT_EQ(Printed, "\tstswp.w\tr1[-4], r2");

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
  raw_string_ostream SubImm8OS(Printed);
  InstPrinter->printInst(&SubImm8, /*Address=*/0, /*Annot=*/"", *STI,
                         SubImm8OS);
  SubImm8OS.flush();
  EXPECT_EQ(Printed, "\tsub\tr1, -1");

  Printed.clear();
  raw_string_ostream SubSPImm8OS(Printed);
  InstPrinter->printInst(&SubSPImm8, /*Address=*/0, /*Annot=*/"", *STI,
                         SubSPImm8OS);
  SubSPImm8OS.flush();
  EXPECT_EQ(Printed, "\tsub\tsp, -4");

  Printed.clear();
  raw_string_ostream SubShiftOS(Printed);
  InstPrinter->printInst(&SubShift, /*Address=*/0, /*Annot=*/"", *STI,
                         SubShiftOS);
  SubShiftOS.flush();
  EXPECT_EQ(Printed, "\tsub\tr1, r2, r3 << 2");

  Printed.clear();
  raw_string_ostream SubRegImm16OS(Printed);
  InstPrinter->printInst(&SubRegImm16, /*Address=*/0, /*Annot=*/"", *STI,
                         SubRegImm16OS);
  SubRegImm16OS.flush();
  EXPECT_EQ(Printed, "\tsub\tr1, r2, -1");

  Printed.clear();
  raw_string_ostream SubImmOS(Printed);
  InstPrinter->printInst(&SubImm, /*Address=*/0, /*Annot=*/"", *STI,
                         SubImmOS);
  SubImmOS.flush();
  EXPECT_EQ(Printed, "\tsub\tr1, -1");

  Printed.clear();
  raw_string_ostream SubEqOS(Printed);
  InstPrinter->printInst(&SubEq, /*Address=*/0, /*Annot=*/"", *STI, SubEqOS);
  SubEqOS.flush();
  EXPECT_EQ(Printed, "\tsubeq\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream SubAlOS(Printed);
  InstPrinter->printInst(&SubAl, /*Address=*/0, /*Annot=*/"", *STI, SubAlOS);
  SubAlOS.flush();
  EXPECT_EQ(Printed, "\tsubal\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream SubEqImmOS(Printed);
  InstPrinter->printInst(&SubEqImm, /*Address=*/0, /*Annot=*/"", *STI,
                         SubEqImmOS);
  SubEqImmOS.flush();
  EXPECT_EQ(Printed, "\tsubeq\tr1, -1");

  Printed.clear();
  raw_string_ostream SubAlImmOS(Printed);
  InstPrinter->printInst(&SubAlImm, /*Address=*/0, /*Annot=*/"", *STI,
                         SubAlImmOS);
  SubAlImmOS.flush();
  EXPECT_EQ(Printed, "\tsubal\tr1, -1");

  Printed.clear();
  raw_string_ostream SubfEqImmOS(Printed);
  InstPrinter->printInst(&SubfEqImm, /*Address=*/0, /*Annot=*/"", *STI,
                         SubfEqImmOS);
  SubfEqImmOS.flush();
  EXPECT_EQ(Printed, "\tsubfeq\tr1, -1");

  Printed.clear();
  raw_string_ostream SubfAlImmOS(Printed);
  InstPrinter->printInst(&SubfAlImm, /*Address=*/0, /*Annot=*/"", *STI,
                         SubfAlImmOS);
  SubfAlImmOS.flush();
  EXPECT_EQ(Printed, "\tsubfal\tr1, -1");

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
  raw_string_ostream XchgOS(Printed);
  InstPrinter->printInst(&Xchg, /*Address=*/0, /*Annot=*/"", *STI, XchgOS);
  XchgOS.flush();
  EXPECT_EQ(Printed, "\txchg\tr1, r2, r3");

  Printed.clear();
  raw_string_ostream MovOS(Printed);
  InstPrinter->printInst(&Mov, /*Address=*/0, /*Annot=*/"", *STI, MovOS);
  MovOS.flush();
  EXPECT_EQ(Printed, "\tmov\tr1, r2");

  Printed.clear();
  raw_string_ostream MovEqOS(Printed);
  InstPrinter->printInst(&MovEq, /*Address=*/0, /*Annot=*/"", *STI, MovEqOS);
  MovEqOS.flush();
  EXPECT_EQ(Printed, "\tmoveq\tr1, r2");

  Printed.clear();
  raw_string_ostream MovAlOS(Printed);
  InstPrinter->printInst(&MovAl, /*Address=*/0, /*Annot=*/"", *STI, MovAlOS);
  MovAlOS.flush();
  EXPECT_EQ(Printed, "\tmoval\tr1, r2");

  Printed.clear();
  raw_string_ostream MovEqImmOS(Printed);
  InstPrinter->printInst(&MovEqImm, /*Address=*/0, /*Annot=*/"", *STI,
                         MovEqImmOS);
  MovEqImmOS.flush();
  EXPECT_EQ(Printed, "\tmoveq\tr1, -1");

  Printed.clear();
  raw_string_ostream MovAlImmOS(Printed);
  InstPrinter->printInst(&MovAlImm, /*Address=*/0, /*Annot=*/"", *STI,
                         MovAlImmOS);
  MovAlImmOS.flush();
  EXPECT_EQ(Printed, "\tmoval\tr1, -1");

  Printed.clear();
  raw_string_ostream MovImmOS(Printed);
  InstPrinter->printInst(&MovImm, /*Address=*/0, /*Annot=*/"", *STI, MovImmOS);
  MovImmOS.flush();
  EXPECT_EQ(Printed, "\tmov\tr1, -1");

  Printed.clear();
  raw_string_ostream MovImm21OS(Printed);
  InstPrinter->printInst(&MovImm21, /*Address=*/0, /*Annot=*/"", *STI,
                         MovImm21OS);
  MovImm21OS.flush();
  EXPECT_EQ(Printed, "\tmov\tr1, 32");

  Printed.clear();
  raw_string_ostream MovhOS(Printed);
  InstPrinter->printInst(&Movh, /*Address=*/0, /*Annot=*/"", *STI, MovhOS);
  MovhOS.flush();
  EXPECT_EQ(Printed, "\tmovh\tr1, 1");

  SourceMgr SrcMgr;
  SrcMgr.AddNewSourceBuffer(
      MemoryBuffer::getMemBuffer(
          "subhh.w r1, r2:t, r3:b\n"
          "xchg r1, r2, r3\n"
          "nop\nfrs\nabs r1\nacr r1\nacall 4\nadc r1, r2, r3\naddabs r1, r2, r3\nadd r1, r2\naddal r1, r2, r3\naddcc r1, r2, r3\naddcs r1, r2, r3\naddeq r1, r2, r3\naddge r1, r2, r3\naddgt r1, r2, r3\naddhi r1, r2, r3\naddhs r1, r2, r3\naddle r1, r2, r3\naddlo r1, r2, r3\naddls r1, r2, r3\naddlt r1, r2, r3\naddmi r1, r2, r3\naddne r1, r2, r3\naddpl r1, r2, r3\naddqs r1, r2, r3\naddvc r1, r2, r3\naddvs r1, r2, r3\nand r1, r2\nandal r1, r2, r3\nandcc r1, r2, r3\nandcs r1, r2, r3\nandeq r1, r2, r3\nandge r1, r2, r3\nandgt r1, r2, r3\nandhi r1, r2, r3\nandhs r1, r2, r3\nandle r1, r2, r3\nandlo r1, r2, r3\nandls r1, r2, r3\nandlt r1, r2, r3\nandmi r1, r2, r3\nandne r1, r2, r3\nandpl r1, r2, r3\nandqs r1, r2, r3\nandvc r1, r2, r3\nandvs r1, r2, r3\nandh r1, 1\nandh r1, 1, coh\nandl r1, 1\nandl r1, 1, coh\nandn r1, r2\nasr r1, r2, r3\nbfexts r1, r2, 3, 5\nbfextu r1, r2, 3, 5\nbfins r1, r2, 3, 5\nbld r1, 1\nbrev r1\nbst r1, 1\nbreakpoint\ncbr r1, 1\ncasts.b r1\ncasts.h r1\ncastu.b r1\ncastu.h r1\nclz r1, r2\ncom r1\ncpc r1\ncpc r1, r2\ncp.b r1, r2\ncp.h r1, r2\ncp.w r1, r2\ncp.w r1, -1\ncp.w r1, 32\ncsrfcz 1\ncsrf 1\ndivs r2, r3, r4\ndivu r2, r3, r4\nneg r1\neor r1, r2\neoral r1, r2, r3\neorcc r1, r2, r3\neorcs r1, r2, r3\neoreq r1, r2, r3\neorge r1, r2, r3\neorgt r1, r2, r3\neorhi r1, r2, r3\neorhs r1, r2, r3\neorle r1, r2, r3\neorlo r1, r2, r3\neorls r1, r2, r3\neorlt r1, r2, r3\neormi r1, r2, r3\neorne r1, r2, r3\neorpl r1, r2, r3\neorqs r1, r2, r3\neorvc r1, r2, r3\neorvs r1, r2, r3\neorh r1, 1\neorl r1, 1\nicall r1\nincjosp -1\nlsl r1, r2, r3\nlsr r1, r2, r3\nmax r1, r2, r3\nmin r1, r2, r3\nmfdr r1, 4\nmfsr r1, 4\nmoval r1, r2\nmovcc r1, r2\nmovcs r1, r2\nmoveq r1, r2\nmovge r1, r2\nmovgt r1, r2\nmovhi r1, r2\nmovhs r1, r2\nmovle r1, r2\nmovlo r1, r2\nmovls r1, r2\nmovlt r1, r2\nmovmi r1, r2\nmovne r1, r2\nmovpl r1, r2\nmovqs r1, r2\nmovvc r1, r2\nmovvs r1, r2\nmoveq r1, -1\nmoval r1, -1\nmul r1, r2\nmul r1, r2, -1\nmul r1, r2, r3\nmuls.d r2, r3, r4\nmulu.d r2, r3, r4\nmusfr r1\nmustr r1\nmtdr 4, r1\nmtsr 4, r1\nmemc 4, 1\nmems 4, 1\nmemt 4, 1\nor r1, r2\noral r1, r2, r3\norcc r1, r2, r3\norcs r1, r2, r3\noreq r1, r2, r3\norge r1, r2, r3\norgt r1, r2, r3\norhi r1, r2, r3\norhs r1, r2, r3\norle r1, r2, r3\norlo r1, r2, r3\norls r1, r2, r3\norlt r1, r2, r3\normi r1, r2, r3\norne r1, r2, r3\norpl r1, r2, r3\norqs r1, r2, r3\norvc r1, r2, r3\norvs r1, r2, r3\norh r1, 1\norl r1, 1\npopjc\npushjc\nretd\nrete\nret\nretal lr\nretcc lr\nretcs lr\nretlo lr\nretge lr\nretlt lr\nretmi lr\nretpl lr\nretls lr\nretgt lr\nretle lr\nrethi lr\nretvs lr\nretvc lr\nretqs lr\nreths lr\nreteq lr\nretne lr\nretj\nrets\nretss\nrol r1\nror r1\nrsub r1, r2\nrsubal r1, -1\nrsubcc r1, -1\nrsubcs r1, -1\nrsubeq r1, -1\nrsubge r1, -1\nrsubgt r1, -1\nrsubhi r1, -1\nrsubhs r1, -1\nrsuble r1, -1\nrsublo r1, -1\nrsubls r1, -1\nrsublt r1, -1\nrsubmi r1, -1\nrsubne r1, -1\nrsubpl r1, -1\nrsubqs r1, -1\nrsubvc r1, -1\nrsubvs r1, -1\nsbc r1, r2, r3\nsbr r1, 1\nscall\nsscall\nscr r1\nsral r1\nsrcc r1\nsrcs r1\nsreq r1\nsrge r1\nsrgt r1\nsrhi r1\nsrhs r1\nsrle r1\nsrlo r1\nsrls r1\nsrlt r1\nsrmi r1\nsrne r1\nsrpl r1\nsrqs r1\nsrvc r1\nsrvs r1\nsleep 1\nssrf 1\nsub r1, r2\nsub r1, -1\nsub sp, -4\nsubal r1, r2, r3\nsubcc r1, r2, r3\nsubcs r1, r2, r3\nsubeq r1, r2, r3\nsubge r1, r2, r3\nsubgt r1, r2, r3\nsubhi r1, r2, r3\nsubhs r1, r2, r3\nsuble r1, r2, r3\nsublo r1, r2, r3\nsubls r1, r2, r3\nsublt r1, r2, r3\nsubmi r1, r2, r3\nsubne r1, r2, r3\nsubpl r1, r2, r3\nsubqs r1, r2, r3\nsubvc r1, r2, r3\nsubvs r1, r2, r3\nsubal r1, -1\nsubcc r1, -1\nsubcs r1, -1\nsubeq r1, -1\nsubge r1, -1\nsubgt r1, -1\nsubhi r1, -1\nsubhs r1, -1\nsuble r1, -1\nsublo r1, -1\nsubls r1, -1\nsublt r1, -1\nsubmi r1, -1\nsubne r1, -1\nsubpl r1, -1\nsubqs r1, -1\nsubvc r1, -1\nsubvs r1, -1\nsubfal r1, -1\nsubfcc r1, -1\nsubfcs r1, -1\nsubfeq r1, -1\nsubfge r1, -1\nsubfgt r1, -1\nsubfhi r1, -1\nsubfhs r1, -1\nsubfle r1, -1\nsubflo r1, -1\nsubfls r1, -1\nsubflt r1, -1\nsubfmi r1, -1\nsubfne r1, -1\nsubfpl r1, -1\nsubfqs r1, -1\nsubfvc r1, -1\nsubfvs r1, -1\nswap.bh r1\nswap.b r1\nswap.h r1\nsync 1\ntlbr\ntlbs\ntlbw\ntnbz r1\ntst r1, r2\nmov r1, r2\nmov r1, -1\nmov r1, 128\nmovh r1, 1\n"),
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

  SourceMgr SubRegImmSrcMgr;
  SubRegImmSrcMgr.AddNewSourceBuffer(
      MemoryBuffer::getMemBuffer(
          "sub r1, r2, -1\nsub r1, r2, r3 << 2\nsatadd.w r1, r2, r3\n"
          "satsub.h r1, r2, r3\nsatsub.w r1, r2, r3\n"
          "satsub.w r1, r2, -1\nsatrnds r1 >> 2, 3\n"
          "satrndu r1 >> 2, 3\nsats r1 >> 2, 3\nsatu r1 >> 2, 3\n"),
      SMLoc());

  MCContext SubRegImmParseCtx(TT, *MAI, *MRI, *STI, &SubRegImmSrcMgr);
  std::unique_ptr<MCObjectFileInfo> SubRegImmMOFI(
      TheTarget->createMCObjectFileInfo(SubRegImmParseCtx, /*PIC=*/false));
  SubRegImmParseCtx.setObjectFileInfo(SubRegImmMOFI.get());

  std::unique_ptr<MCStreamer> SubRegImmStreamer(
      TheTarget->createNullStreamer(SubRegImmParseCtx));
  std::unique_ptr<MCAsmParser> SubRegImmParser(createMCAsmParser(
      SubRegImmSrcMgr, SubRegImmParseCtx, *SubRegImmStreamer, *MAI));
  std::unique_ptr<MCTargetAsmParser> SubRegImmTargetParser(
      TheTarget->createMCAsmParser(*STI, *SubRegImmParser, *MII));
  ASSERT_NE(SubRegImmTargetParser.get(), nullptr);
  SubRegImmParser->setTargetParser(*SubRegImmTargetParser);
  EXPECT_FALSE(SubRegImmParser->Run(/*NoInitialTextSection=*/false));

  SourceMgr StoreSrcMgr;
  StoreSrcMgr.AddNewSourceBuffer(
      MemoryBuffer::getMemBuffer(
          "ld.w r1, r2++\nld.w r1, --r2\nld.w r1, r2[12]\n"
          "ld.w r1, r2[-1]\nld.w r1, r2[r3 << 2]\n"
          "ld.w r1, r2[r3:l << 2]\n"
          "ld.weq r1, r2[12]\nld.wal r1, r2[12]\n"
          "ld.d r2, r1++\nld.d r2, --r1\nld.d r2, r1\n"
          "ld.d r2, r1[-1]\nld.d r2, r1[r3 << 3]\n"
          "lddpc r2, pc[12]\nlddsp r2, sp[12]\n"
          "ldins.b r1:u, r2[-1]\nldins.h r1:t, r2[-2]\n"
          "ld.sb r1, r2[-1]\nld.sb r1, r2[r3 << 2]\n"
          "ld.sbeq r1, r2[3]\nld.sbal r1, r2[3]\n"
          "ld.ub r1, r2++\nld.ub r1, --r2\nld.ub r1, r2[3]\n"
          "ld.ub r1, r2[-1]\nld.ub r1, r2[r3 << 2]\n"
          "ld.ubeq r1, r2[3]\nld.ubal r1, r2[3]\n"
          "ld.sh r1, r2++\nld.sh r1, --r2\nld.sh r1, r2[6]\n"
          "ld.sh r1, r2[-1]\nld.sh r1, r2[r3 << 2]\n"
          "ld.sheq r1, r2[6]\nld.shal r1, r2[6]\n"
          "ld.uh r1, r2++\nld.uh r1, --r2\nld.uh r1, r2[6]\n"
          "ld.uh r1, r2[-1]\nld.uh r1, r2[r3 << 2]\n"
          "ld.uheq r1, r2[6]\nld.uhal r1, r2[6]\n"
          "st.b r1++, r2\nst.b --r1, r2\nst.b r1[3], r2\n"
          "st.b r1[-1], r2\nst.b r1[r2 << 3], r4\n"
          "st.beq r1[3], r2\nst.bal r1[3], r2\n"
          "st.d r1++, r2\nst.d --r1, r2\nst.d r1, r2\n"
          "st.d r1[-1], r2\nst.d r1[r2 << 3], r4\n"
          "st.h r1++, r2\nst.h --r1, r2\nst.h r1[6], r2\n"
          "st.h r1[-1], r2\nst.h r1[r2 << 3], r4\n"
          "st.heq r1[6], r2\nst.hal r1[6], r2\n"
          "st.w r1++, r2\nst.w --r1, r2\nst.w r1[12], r2\n"
          "st.w r1[-1], r2\nst.w r1[r2 << 3], r4\n"
          "st.weq r1[12], r2\nst.wal r1[12], r2\n"
          "stcond r1[-1], r2\nstdsp sp[12], r2\n"
          "sthh.w r1[12], r2:t, r3:b\n"
          "sthh.w r1[r4 << 3], r2:t, r3:b\n"
          "stswp.h r1[-2], r2\nstswp.w r1[-4], r2\n"),
      SMLoc());

  MCContext StoreParseCtx(TT, *MAI, *MRI, *STI, &StoreSrcMgr);
  std::unique_ptr<MCObjectFileInfo> StoreMOFI(
      TheTarget->createMCObjectFileInfo(StoreParseCtx, /*PIC=*/false));
  StoreParseCtx.setObjectFileInfo(StoreMOFI.get());

  std::unique_ptr<MCStreamer> StoreStreamer(
      TheTarget->createNullStreamer(StoreParseCtx));
  std::unique_ptr<MCAsmParser> StoreParser(
      createMCAsmParser(StoreSrcMgr, StoreParseCtx, *StoreStreamer, *MAI));
  std::unique_ptr<MCTargetAsmParser> StoreTargetParser(
      TheTarget->createMCAsmParser(*STI, *StoreParser, *MII));
  ASSERT_NE(StoreTargetParser.get(), nullptr);
  StoreParser->setTargetParser(*StoreTargetParser);
  EXPECT_FALSE(StoreParser->Run(/*NoInitialTextSection=*/false));

  SourceMgr PrefSrcMgr;
  PrefSrcMgr.AddNewSourceBuffer(
      MemoryBuffer::getMemBuffer("cache r1[-1], 1\npref r1[-1]\n"), SMLoc());

  MCContext PrefParseCtx(TT, *MAI, *MRI, *STI, &PrefSrcMgr);
  std::unique_ptr<MCObjectFileInfo> PrefMOFI(
      TheTarget->createMCObjectFileInfo(PrefParseCtx, /*PIC=*/false));
  PrefParseCtx.setObjectFileInfo(PrefMOFI.get());

  std::unique_ptr<MCStreamer> PrefStreamer(
      TheTarget->createNullStreamer(PrefParseCtx));
  std::unique_ptr<MCAsmParser> PrefParser(
      createMCAsmParser(PrefSrcMgr, PrefParseCtx, *PrefStreamer, *MAI));
  std::unique_ptr<MCTargetAsmParser> PrefTargetParser(
      TheTarget->createMCAsmParser(*STI, *PrefParser, *MII));
  ASSERT_NE(PrefTargetParser.get(), nullptr);
  PrefParser->setTargetParser(*PrefTargetParser);
  EXPECT_FALSE(PrefParser->Run(/*NoInitialTextSection=*/false));

  SourceMgr PsadSrcMgr;
  PsadSrcMgr.AddNewSourceBuffer(
      MemoryBuffer::getMemBuffer(
          "pabs.sb r1, r2\npabs.sh r1, r2\n"
          "packsh.ub r1, r2, r3\npacksh.sb r1, r2, r3\n"
          "packw.sh r1, r2, r3\n"
          "padd.b r1, r2, r3\npadd.h r1, r2, r3\n"
          "paddh.ub r1, r2, r3\npaddh.sh r1, r2, r3\n"
          "padds.ub r1, r2, r3\npadds.sb r1, r2, r3\n"
          "padds.uh r1, r2, r3\npadds.sh r1, r2, r3\n"
          "paddx.h r1, r2, r3\npaddxh.sh r1, r2, r3\n"
          "paddxs.uh r1, r2, r3\npaddxs.sh r1, r2, r3\n"
          "pasr.b r1, r2, 3\npasr.h r1, r2, 15\n"
          "plsl.b r1, r2, 3\nplsl.h r1, r2, 15\n"
          "plsr.b r1, r2, 3\nplsr.h r1, r2, 15\n"
          "pavg.ub r1, r2, r3\npavg.sh r1, r2, r3\n"
          "pmax.ub r1, r2, r3\npmax.sh r1, r2, r3\n"
          "pmin.ub r1, r2, r3\npmin.sh r1, r2, r3\n"
          "psad r1, r2, r3\npsub.b r1, r2, r3\npsub.h r1, r2, r3\n"
          "psubh.ub r1, r2, r3\npsubh.sh r1, r2, r3\n"
          "psubs.ub r1, r2, r3\npsubs.sb r1, r2, r3\n"
          "psubs.uh r1, r2, r3\npsubs.sh r1, r2, r3\n"
          "psubx.h r1, r2, r3\npsubxh.sh r1, r2, r3\n"
          "psubxs.uh r1, r2, r3\npsubxs.sh r1, r2, r3\n"),
      SMLoc());

  MCContext PsadParseCtx(TT, *MAI, *MRI, *STI, &PsadSrcMgr);
  std::unique_ptr<MCObjectFileInfo> PsadMOFI(
      TheTarget->createMCObjectFileInfo(PsadParseCtx, /*PIC=*/false));
  PsadParseCtx.setObjectFileInfo(PsadMOFI.get());

  std::unique_ptr<MCStreamer> PsadStreamer(
      TheTarget->createNullStreamer(PsadParseCtx));
  std::unique_ptr<MCAsmParser> PsadParser(
      createMCAsmParser(PsadSrcMgr, PsadParseCtx, *PsadStreamer, *MAI));
  std::unique_ptr<MCTargetAsmParser> PsadTargetParser(
      TheTarget->createMCAsmParser(*STI, *PsadParser, *MII));
  ASSERT_NE(PsadTargetParser.get(), nullptr);
  PsadParser->setTargetParser(*PsadTargetParser);
  EXPECT_FALSE(PsadParser->Run(/*NoInitialTextSection=*/false));

  SourceMgr PsubaddSrcMgr;
  PsubaddSrcMgr.AddNewSourceBuffer(
      MemoryBuffer::getMemBuffer(
          "paddsub.h r1, r2:t, r3:b\npaddsubh.sh r1, r2:t, r3:b\n"
          "paddsubs.uh r1, r2:t, r3:b\npaddsubs.sh r1, r2:t, r3:b\n"
          "psubadd.h r1, r2:t, r3:b\npsubaddh.sh r1, r2:t, r3:b\n"
          "psubadds.uh r1, r2:t, r3:b\npsubadds.sh r1, r2:t, r3:b\n"
          "punpckub.h r1, r2:b\npunpcksb.h r1, r2:t\n"),
      SMLoc());

  MCContext PsubaddParseCtx(TT, *MAI, *MRI, *STI, &PsubaddSrcMgr);
  std::unique_ptr<MCObjectFileInfo> PsubaddMOFI(
      TheTarget->createMCObjectFileInfo(PsubaddParseCtx, /*PIC=*/false));
  PsubaddParseCtx.setObjectFileInfo(PsubaddMOFI.get());

  std::unique_ptr<MCStreamer> PsubaddStreamer(
      TheTarget->createNullStreamer(PsubaddParseCtx));
  std::unique_ptr<MCAsmParser> PsubaddParser(createMCAsmParser(
      PsubaddSrcMgr, PsubaddParseCtx, *PsubaddStreamer, *MAI));
  std::unique_ptr<MCTargetAsmParser> PsubaddTargetParser(
      TheTarget->createMCAsmParser(*STI, *PsubaddParser, *MII));
  ASSERT_NE(PsubaddTargetParser.get(), nullptr);
  PsubaddParser->setTargetParser(*PsubaddTargetParser);
  EXPECT_FALSE(PsubaddParser->Run(/*NoInitialTextSection=*/false));
}

} // namespace
