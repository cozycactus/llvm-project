//===-- AVR32MCCodeEmitter.cpp - AVR32 Machine Code Emitter ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32MCTargetDesc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
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
