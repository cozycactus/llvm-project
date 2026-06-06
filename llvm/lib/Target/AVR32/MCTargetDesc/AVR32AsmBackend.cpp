//===-- AVR32AsmBackend.cpp - AVR32 Assembler Backend ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32FixupKinds.h"
#include "AVR32MCTargetDesc.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

class AVR32AsmBackend : public MCAsmBackend {
  bool LinkRelax;

public:
  AVR32AsmBackend(const MCSubtargetInfo &STI)
      : MCAsmBackend(llvm::endianness::big),
        LinkRelax(STI.hasFeature(AVR32::FeatureRelax)) {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createAVR32ELFObjectWriter(ELF::ELFOSABI_NONE);
  }

  std::optional<bool> evaluateFixup(const MCFragment &F, MCFixup &Fixup,
                                    MCValue &, uint64_t &Value) override {
    if (Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_7w_pcrel) ||
        Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_16w_pcrel)) {
      assert(Asm && "missing MCAssembler");
      Value = (Asm->getFragmentOffset(F) + Fixup.getOffset()) % 4;
    }
    return {};
  }

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override {
    if (IsResolved &&
        Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_16b_pcrel))
      IsResolved = !LinkRelax;
    maybeAddReloc(F, Fixup, Target, Value, IsResolved);
    if (mc::isRelocation(Fixup.getKind()))
      return;
    if (!Value)
      return;

    if (Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_22h_pcrel)) {
      int64_t SignedValue = static_cast<int64_t>(Value);
      if (SignedValue & 1) {
        getContext().reportError(Fixup.getLoc(),
                                 "fixup value must be 2-byte aligned");
        return;
      }
      if (!isInt<22>(SignedValue)) {
        getContext().reportError(Fixup.getLoc(), "fixup value out of range");
        return;
      }

      int64_t Encoded = SignedValue >> 1;
      uint32_t Disp = static_cast<uint32_t>(Encoded) & 0x1fffff;
      uint32_t Word = support::endian::read32be(Data);
      Word &= ~0x1e10ffff;
      Word |=
          (Disp & 0xffff) | ((Disp & 0x10000) << 4) | ((Disp & 0x1e0000) << 8);
      support::endian::write32be(Data, Word);
      return;
    }

    if (Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_9h_pcrel)) {
      int64_t SignedValue = static_cast<int64_t>(Value);
      if (SignedValue & 1) {
        getContext().reportError(Fixup.getLoc(),
                                 "fixup value must be 2-byte aligned");
        return;
      }
      if (!isInt<9>(SignedValue)) {
        getContext().reportError(Fixup.getLoc(), "fixup value out of range");
        return;
      }

      int64_t Encoded = SignedValue >> 1;
      uint16_t Disp = static_cast<uint16_t>(Encoded) & 0xff;
      uint16_t Word = support::endian::read16be(Data);
      Word &= ~0x0ff0;
      Word |= Disp << 4;
      support::endian::write16be(Data, Word);
      return;
    }

    if (Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_11h_pcrel)) {
      int64_t SignedValue = static_cast<int64_t>(Value);
      if (SignedValue & 1) {
        getContext().reportError(Fixup.getLoc(),
                                 "fixup value must be 2-byte aligned");
        return;
      }
      if (!isInt<11>(SignedValue)) {
        getContext().reportError(Fixup.getLoc(), "fixup value out of range");
        return;
      }

      int64_t Encoded = SignedValue >> 1;
      uint16_t Disp = static_cast<uint16_t>(Encoded) & 0x3ff;
      uint16_t Word = support::endian::read16be(Data);
      Word &= ~0x0ff3;
      Word |= ((Disp & 0xff) << 4) | ((Disp & 0x300) >> 8);
      support::endian::write16be(Data, Word);
      return;
    }

    if (Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_7w_pcrel)) {
      int64_t SignedValue = static_cast<int64_t>(Value);
      if (SignedValue & 3) {
        getContext().reportError(Fixup.getLoc(),
                                 "fixup value must be 4-byte aligned");
        return;
      }
      if (SignedValue < 0 || SignedValue > 508) {
        getContext().reportError(Fixup.getLoc(), "fixup value out of range");
        return;
      }

      uint16_t Disp = static_cast<uint16_t>(SignedValue >> 2) & 0x7f;
      uint16_t Word = support::endian::read16be(Data);
      Word &= ~0x07f0;
      Word |= Disp << 4;
      support::endian::write16be(Data, Word);
      return;
    }

    if (Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_16w_pcrel)) {
      int64_t SignedValue = static_cast<int64_t>(Value);
      if (SignedValue & 3) {
        getContext().reportError(Fixup.getLoc(),
                                 "fixup value must be 4-byte aligned");
        return;
      }
      if (SignedValue < -131072 || SignedValue > 131068) {
        getContext().reportError(Fixup.getLoc(), "fixup value out of range");
        return;
      }

      uint16_t Disp = static_cast<uint16_t>(SignedValue >> 2);
      support::endian::write16be(Data + 2, Disp);
      return;
    }

    if (Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_16b_pcrel)) {
      int64_t SignedValue = static_cast<int64_t>(Value);
      if (!isInt<16>(SignedValue)) {
        getContext().reportError(Fixup.getLoc(), "fixup value out of range");
        return;
      }

      uint32_t Word = support::endian::read32be(Data) & ~0xffff;
      Word |= static_cast<uint16_t>(SignedValue);
      support::endian::write32be(Data, Word);
      return;
    }

    if (Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_21s)) {
      int64_t SignedValue = static_cast<int64_t>(Value);
      if (!isInt<21>(SignedValue)) {
        getContext().reportError(Fixup.getLoc(), "fixup value out of range");
        return;
      }

      uint32_t Encoded = static_cast<uint32_t>(SignedValue) & 0x1fffff;
      uint32_t Word = support::endian::read32be(Data);
      Word &= ~0x1e10ffff;
      Word |= (Encoded & 0xffff) | ((Encoded & 0x10000) << 4) |
              ((Encoded & 0x1e0000) << 8);
      support::endian::write32be(Data, Word);
      return;
    }

    if (Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_hi16) ||
        Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_lo16)) {
      uint16_t Encoded =
          Fixup.getKind() == static_cast<MCFixupKind>(AVR32::fixup_hi16)
              ? static_cast<uint16_t>(Value >> 16)
              : static_cast<uint16_t>(Value);
      support::endian::write16be(Data, Encoded);
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
        {"fixup_22h_pcrel", 0, 32, 0}, {"fixup_9h_pcrel", 0, 16, 0},
        {"fixup_11h_pcrel", 0, 16, 0}, {"fixup_7w_pcrel", 0, 16, 0},
        {"fixup_16w_pcrel", 0, 32, 0}, {"fixup_16b_pcrel", 0, 32, 0},
        {"fixup_21s", 0, 32, 0},       {"fixup_hi16", 0, 16, 0},
        {"fixup_lo16", 0, 16, 0},
    };

    if (mc::isRelocation(Kind))
      return {};

    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);

    assert(unsigned(Kind - FirstTargetFixupKind) < AVR32::NumTargetFixupKinds &&
           "invalid AVR32 fixup kind");
    return Infos[Kind - FirstTargetFixupKind];
  }

  std::optional<MCFixupKind> getFixupKind(StringRef Name) const override {
    unsigned Type = llvm::StringSwitch<unsigned>(Name)
#define ELF_RELOC(X, Y) .Case(#X, Y)
#include "llvm/BinaryFormat/ELFRelocs/AVR32.def"
#undef ELF_RELOC
                        .Default(-1u);
    if (Type == -1u)
      return std::nullopt;
    return static_cast<MCFixupKind>(FirstLiteralRelocationKind + Type);
  }

  bool relaxAlign(MCFragment &F, unsigned &Size) override {
    auto *Sec = F.getParent();
    if (!LinkRelax || F.getLayoutOrder() <= Sec->firstLinkerRelaxable())
      return false;

    if (F.getAlignment() <= 2)
      return false;

    Size = F.getAlignment().value() - 2;
    auto *Expr = MCConstantExpr::create(Log2(F.getAlignment()), getContext());
    MCFixup Fixup = MCFixup::create(
        0, Expr, FirstLiteralRelocationKind + ELF::R_AVR32_ALIGN);
    F.setVarFixups({Fixup});
    F.setLinkerRelaxable();
    return true;
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    if (Count % 2 != 0)
      return false;

    for (uint64_t I = 0; I != Count; I += 2)
      OS.write("\xd7\x03", 2);

    return true;
  }
};

} // end anonymous namespace

MCAsmBackend *llvm::createAVR32MCAsmBackend(const Target &,
                                            const MCSubtargetInfo &STI,
                                            const MCRegisterInfo &,
                                            const MCTargetOptions &) {
  return new AVR32AsmBackend(STI);
}
