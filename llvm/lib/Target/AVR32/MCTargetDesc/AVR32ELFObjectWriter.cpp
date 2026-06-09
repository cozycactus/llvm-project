//===-- AVR32ELFObjectWriter.cpp - AVR32 ELF Writer -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32FixupKinds.h"
#include "AVR32MCTargetDesc.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class AVR32ELFObjectWriter : public MCELFObjectTargetWriter {
public:
  explicit AVR32ELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI, ELF::EM_AVR32,
                                /*HasRelocationAddend=*/true) {}

protected:
  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    switch (Fixup.getKind()) {
    case AVR32::fixup_22h_pcrel:
      return ELF::R_AVR32_22H_PCREL;
    case AVR32::fixup_9h_pcrel:
      return ELF::R_AVR32_9H_PCREL;
    case AVR32::fixup_11h_pcrel:
      return ELF::R_AVR32_11H_PCREL;
    case AVR32::fixup_16w_pcrel:
      return ELF::R_AVR32_18W_PCREL;
    case AVR32::fixup_16b_pcrel:
      return ELF::R_AVR32_16B_PCREL;
    case AVR32::fixup_cpcall:
      return ELF::R_AVR32_CPCALL;
    case AVR32::fixup_21s:
      return ELF::R_AVR32_21S;
    case AVR32::fixup_hi16:
      return ELF::R_AVR32_HI16;
    case AVR32::fixup_lo16:
      return ELF::R_AVR32_LO16;
    case FK_Data_1:
      return IsPCRel ? ELF::R_AVR32_8_PCREL : ELF::R_AVR32_8;
    case FK_Data_2:
      return IsPCRel ? ELF::R_AVR32_16_PCREL : ELF::R_AVR32_16;
    case FK_Data_4:
      if (!IsPCRel && Target.getSpecifier() == ELF::R_AVR32_32_CPENT)
        return ELF::R_AVR32_32_CPENT;
      return IsPCRel ? ELF::R_AVR32_32_PCREL : ELF::R_AVR32_32;
    default:
      llvm_unreachable("invalid AVR32 fixup kind");
    }
  }
};

} // end anonymous namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createAVR32ELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<AVR32ELFObjectWriter>(OSABI);
}
