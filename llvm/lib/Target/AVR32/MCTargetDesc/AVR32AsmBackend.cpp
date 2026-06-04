//===-- AVR32AsmBackend.cpp - AVR32 Assembler Backend ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32FixupKinds.h"
#include "AVR32MCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class AVR32AsmBackend : public MCAsmBackend {
public:
  AVR32AsmBackend() : MCAsmBackend(llvm::endianness::big) {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createAVR32ELFObjectWriter(ELF::ELFOSABI_NONE);
  }

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override {
    maybeAddReloc(F, Fixup, Target, Value, IsResolved);
    if (!Value)
      return;

    if (Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_22h_pcrel)) {
      int64_t Encoded = static_cast<int64_t>(Value) >> 1;
      uint32_t Disp = static_cast<uint32_t>(Encoded) & 0x1fffff;
      uint32_t Word = support::endian::read32be(Data);
      Word |=
          (Disp & 0xffff) | ((Disp & 0x10000) << 4) | ((Disp & 0x1e0000) << 8);
      support::endian::write32be(Data, Word);
      return;
    }

    MCFixupKindInfo Info = getFixupKindInfo(Fixup.getKind());
    assert(Info.TargetOffset == 0 && "unsupported AVR32 fixup offset");
    assert(Info.TargetSize % 8 == 0 && "invalid AVR32 fixup size");

    switch (Info.TargetSize) {
    case 8:
      *Data = Value;
      break;
    case 16:
      support::endian::write16be(Data, Value);
      break;
    case 32:
      support::endian::write32be(Data, Value);
      break;
    default:
      llvm_unreachable("unsupported AVR32 fixup size");
    }
  }

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override {
    static const MCFixupKindInfo Infos[AVR32::NumTargetFixupKinds] = {
        {"fixup_22h_pcrel", 0, 32, 0},
    };

    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);

    assert(unsigned(Kind - FirstTargetFixupKind) < AVR32::NumTargetFixupKinds &&
           "invalid AVR32 fixup kind");
    return Infos[Kind - FirstTargetFixupKind];
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    if (Count == 0)
      return true;
    return false;
  }
};

} // end anonymous namespace

MCAsmBackend *llvm::createAVR32MCAsmBackend(const Target &,
                                            const MCSubtargetInfo &,
                                            const MCRegisterInfo &,
                                            const MCTargetOptions &) {
  return new AVR32AsmBackend();
}
