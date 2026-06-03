//===-- AVR32ELFObjectWriter.cpp - AVR32 ELF Writer -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

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
  unsigned getRelocType(const MCFixup &Fixup, const MCValue &,
                        bool IsPCRel) const override {
    switch (Fixup.getKind()) {
    case FK_Data_1:
      return IsPCRel ? ELF::R_AVR32_8_PCREL : ELF::R_AVR32_8;
    case FK_Data_2:
      return IsPCRel ? ELF::R_AVR32_16_PCREL : ELF::R_AVR32_16;
    case FK_Data_4:
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
