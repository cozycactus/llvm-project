//===-- AVR32MCCodeEmitter.cpp - AVR32 Machine Code Emitter ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32FixupKinds.h"
#include "AVR32MCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/EndianStream.h"

using namespace llvm;

namespace {

class AVR32MCCodeEmitter : public MCCodeEmitter {
  const MCRegisterInfo &MRI;
  const MCInstrInfo &MCII;

public:
  AVR32MCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx)
      : MRI(*Ctx.getRegisterInfo()), MCII(MCII) {}

  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const {
    if (MO.isReg())
      return MRI.getEncodingValue(MO.getReg());
    if (MO.isImm())
      return static_cast<unsigned>(MO.getImm());
    return 0;
  }

  unsigned getSysRegAddrOpValue(const MCInst &MI, unsigned OpNo,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(OpNo);
    if (MO.isImm())
      return static_cast<unsigned>(MO.getImm()) >> 2;
    return 0;
  }

  unsigned getShifted2OpValue(const MCInst &MI, unsigned OpNo,
                              SmallVectorImpl<MCFixup> &Fixups,
                              const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(OpNo);
    if (MO.isImm())
      return static_cast<unsigned>(MO.getImm()) >> 2;
    return 0;
  }

  unsigned getShifted1OpValue(const MCInst &MI, unsigned OpNo,
                              SmallVectorImpl<MCFixup> &Fixups,
                              const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(OpNo);
    if (MO.isImm())
      return static_cast<unsigned>(MO.getImm()) >> 1;
    return 0;
  }

  unsigned getSImm21OpValue(const MCInst &MI, unsigned OpNo,
                            SmallVectorImpl<MCFixup> &Fixups,
                            const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(OpNo);
    if (MO.isImm())
      return static_cast<unsigned>(MO.getImm());

    assert(MO.isExpr() && "expected expression operand");
    if (const auto *Specifier = dyn_cast<MCSpecifierExpr>(MO.getExpr())) {
      MCFixupKind Kind = FK_NONE;
      if (Specifier->getSpecifier() == ELF::R_AVR32_HI16)
        Kind = static_cast<MCFixupKind>(AVR32::fixup_hi16);
      else if (Specifier->getSpecifier() == ELF::R_AVR32_LO16)
        Kind = static_cast<MCFixupKind>(AVR32::fixup_lo16);
      if (Kind != FK_NONE) {
        Fixups.push_back(MCFixup::create(0, MO.getExpr(), Kind,
                                         /*PCRel=*/false));
        return 0;
      }
    }
    Fixups.push_back(MCFixup::create(
        0, MO.getExpr(), static_cast<MCFixupKind>(AVR32::fixup_21s),
        /*PCRel=*/false));
    return 0;
  }

  unsigned getUImm16OpValue(const MCInst &MI, unsigned OpNo,
                            SmallVectorImpl<MCFixup> &Fixups,
                            const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(OpNo);
    if (MO.isImm())
      return static_cast<unsigned>(MO.getImm());

    assert(MO.isExpr() && "expected expression operand");
    const MCExpr *Expr = MO.getExpr();
    MCFixupKind Kind = FK_Data_2;
    if (const auto *Specifier = dyn_cast<MCSpecifierExpr>(Expr)) {
      if (Specifier->getSpecifier() == ELF::R_AVR32_HI16)
        Kind = static_cast<MCFixupKind>(AVR32::fixup_hi16);
      else if (Specifier->getSpecifier() == ELF::R_AVR32_LO16)
        Kind = static_cast<MCFixupKind>(AVR32::fixup_lo16);
    }
    Fixups.push_back(MCFixup::create(0, Expr, Kind, /*PCRel=*/false));
    return 0;
  }

  unsigned getPCRel22HOpValue(const MCInst &MI, unsigned OpNo,
                              SmallVectorImpl<MCFixup> &Fixups,
                              const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(OpNo);
    if (MO.isImm())
      return static_cast<unsigned>(MO.getImm()) >> 1;

    assert(MO.isExpr() && "expected expression operand");
    Fixups.push_back(MCFixup::create(
        0, MO.getExpr(), static_cast<MCFixupKind>(AVR32::fixup_22h_pcrel),
        /*PCRel=*/true));
    return 0;
  }

  unsigned getPCRel9HOpValue(const MCInst &MI, unsigned OpNo,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(OpNo);
    if (MO.isImm())
      return static_cast<unsigned>(MO.getImm()) >> 1;

    assert(MO.isExpr() && "expected expression operand");
    Fixups.push_back(MCFixup::create(
        0, MO.getExpr(), static_cast<MCFixupKind>(AVR32::fixup_9h_pcrel),
        /*PCRel=*/true));
    return 0;
  }

  unsigned getPCRel11HOpValue(const MCInst &MI, unsigned OpNo,
                              SmallVectorImpl<MCFixup> &Fixups,
                              const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(OpNo);
    if (MO.isImm())
      return static_cast<unsigned>(MO.getImm()) >> 1;

    assert(MO.isExpr() && "expected expression operand");
    Fixups.push_back(MCFixup::create(
        0, MO.getExpr(), static_cast<MCFixupKind>(AVR32::fixup_11h_pcrel),
        /*PCRel=*/true));
    return 0;
  }

  unsigned getIncJOSPImmOpValue(const MCInst &MI, unsigned OpNo,
                                SmallVectorImpl<MCFixup> &Fixups,
                                const MCSubtargetInfo &STI) const {
    const MCOperand &MO = MI.getOperand(OpNo);
    if (!MO.isImm())
      return 0;
    int64_t Imm = MO.getImm();
    return Imm > 0 ? Imm - 1 : Imm + 8;
  }

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override {
    uint64_t Bits = getBinaryCodeForInstr(MI, Fixups, STI);
    unsigned Size = MCII.get(MI.getOpcode()).getSize();
    if (Size == 2) {
      support::endian::write<uint16_t>(CB, Bits, llvm::endianness::big);
      return;
    }
    assert(Size == 4 && "unsupported AVR32 instruction size");
    support::endian::write<uint32_t>(CB, Bits, llvm::endianness::big);
  }
};

} // end anonymous namespace

#include "../AVR32GenMCCodeEmitter.inc"

MCCodeEmitter *llvm::createAVR32MCCodeEmitter(const MCInstrInfo &MCII,
                                              MCContext &Ctx) {
  return new AVR32MCCodeEmitter(MCII, Ctx);
}
