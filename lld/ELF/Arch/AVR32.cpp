//===- AVR32.cpp ----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "Symbols.h"
#include "Target.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::ELF;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace lld;
using namespace lld::elf;

namespace {
class AVR32 final : public TargetInfo {
public:
  AVR32(Ctx &ctx) : TargetInfo(ctx) {
    relativeRel = R_AVR32_RELATIVE;
    symbolicRel = R_AVR32_32;
    trapInstr = {0xd6, 0x73, 0xd6, 0x73}; // breakpoint; breakpoint
  }

  uint32_t calcEFlags() const override;
  RelType getDynRel(RelType type) const override;
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  int64_t getImplicitAddend(const uint8_t *buf, RelType type) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};
} // namespace

static uint32_t encodeS21(int64_t val) {
  uint32_t enc = static_cast<uint32_t>(val);
  return (enc & 0xffff) | ((enc & 0x10000) << 4) |
         ((enc & 0x1e0000) << 8);
}

static int64_t decodeS21(uint32_t word) {
  uint32_t enc =
      (word & 0xffff) | ((word >> 4) & 0x10000) | ((word >> 8) & 0x1e0000);
  return SignExtend64<21>(enc);
}

static int64_t getAlignedPCRel(const Relocation &rel, uint64_t val,
                               uint32_t alignment) {
  // AVR32 branch and constant-pool relocations subtract the PC after clearing
  // the low bits implied by the relocation's right shift.
  return SignExtend64(val + (rel.offset & (alignment - 1)), 32);
}

static uint32_t getEFlags(InputFile *file) {
  return cast<ObjFile<ELF32BE>>(file)->getObj().getHeader().e_flags;
}

uint32_t AVR32::calcEFlags() const {
  assert(!ctx.objectFiles.empty());

  uint32_t flags =
      getEFlags(ctx.objectFiles[0]) & (EF_AVR32_LINKRELAX | EF_AVR32_PIC);
  for (InputFile *f : ArrayRef(ctx.objectFiles).slice(1))
    flags &= getEFlags(f) & (EF_AVR32_LINKRELAX | EF_AVR32_PIC);
  return flags;
}

RelType AVR32::getDynRel(RelType type) const {
  return type == symbolicRel ? type : R_AVR32_NONE;
}

RelExpr AVR32::getRelExpr(RelType type, const Symbol &s,
                          const uint8_t *loc) const {
  switch (type) {
  case R_AVR32_NONE:
  case R_AVR32_ALIGN:
  case R_AVR32_DIFF8:
  case R_AVR32_DIFF16:
  case R_AVR32_DIFF32:
    return R_NONE;
  case R_AVR32_8:
  case R_AVR32_16:
  case R_AVR32_32:
  case R_AVR32_21S:
  case R_AVR32_HI16:
  case R_AVR32_LO16:
  case R_AVR32_32_CPENT:
    return R_ABS;
  case R_AVR32_8_PCREL:
  case R_AVR32_16_PCREL:
  case R_AVR32_32_PCREL:
  case R_AVR32_22H_PCREL:
  case R_AVR32_11H_PCREL:
  case R_AVR32_9H_PCREL:
  case R_AVR32_CPCALL:
  case R_AVR32_16_CP:
  case R_AVR32_9W_CP:
    return R_PC;
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unknown relocation (" << type.v
             << ") against symbol " << &s;
    return R_NONE;
  }
}

int64_t AVR32::getImplicitAddend(const uint8_t *buf, RelType type) const {
  switch (type) {
  case R_AVR32_NONE:
  case R_AVR32_ALIGN:
  case R_AVR32_GLOB_DAT:
  case R_AVR32_JMP_SLOT:
    return 0;
  case R_AVR32_8:
  case R_AVR32_8_PCREL:
  case R_AVR32_DIFF8:
    return SignExtend64<8>(*buf);
  case R_AVR32_16:
  case R_AVR32_16_PCREL:
  case R_AVR32_DIFF16:
    return SignExtend64<16>(read16be(buf));
  case R_AVR32_32:
  case R_AVR32_32_PCREL:
  case R_AVR32_DIFF32:
  case R_AVR32_32_CPENT:
  case R_AVR32_RELATIVE:
    return SignExtend64<32>(read32be(buf));
  case R_AVR32_21S:
    return decodeS21(read32be(buf));
  case R_AVR32_22H_PCREL: {
    uint32_t word = read32be(buf);
    return decodeS21(word) << 1;
  }
  case R_AVR32_11H_PCREL: {
    uint16_t word = read16be(buf);
    uint16_t enc = ((word >> 4) & 0xff) | ((word & 0x3) << 8);
    return SignExtend64<10>(enc) << 1;
  }
  case R_AVR32_9H_PCREL: {
    uint16_t word = read16be(buf);
    return SignExtend64<8>((word & 0x0ff0) >> 4) << 1;
  }
  case R_AVR32_HI16:
    return (read32be(buf) & 0xffff) << 16;
  case R_AVR32_LO16:
    return read32be(buf) & 0xffff;
  case R_AVR32_CPCALL:
    return SignExtend64<16>(read32be(buf) & 0xffff) << 2;
  case R_AVR32_16_CP:
    return SignExtend64<16>(read32be(buf) & 0xffff);
  case R_AVR32_9W_CP:
    return ((read16be(buf) & 0x07f0) >> 4) << 2;
  default:
    return TargetInfo::getImplicitAddend(buf, type);
  }
}

void AVR32::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  switch (rel.type) {
  case R_AVR32_NONE:
  case R_AVR32_ALIGN:
  case R_AVR32_DIFF8:
  case R_AVR32_DIFF16:
  case R_AVR32_DIFF32:
    break;
  case R_AVR32_8:
    checkIntUInt(ctx, loc, val, 8, rel);
    *loc = val;
    break;
  case R_AVR32_16:
    checkIntUInt(ctx, loc, val, 16, rel);
    write16be(loc, val);
    break;
  case R_AVR32_32:
    checkIntUInt(ctx, loc, val, 32, rel);
    write32be(loc, val);
    break;
  case R_AVR32_21S: {
    checkInt(ctx, loc, val, 21, rel);
    uint32_t word = read32be(loc) & ~0x1e10ffff;
    write32be(loc, word | encodeS21(SignExtend64(val, 32)));
    break;
  }
  case R_AVR32_HI16: {
    uint32_t word = read32be(loc) & ~0xffff;
    write32be(loc, word | ((val >> 16) & 0xffff));
    break;
  }
  case R_AVR32_LO16: {
    uint32_t word = read32be(loc) & ~0xffff;
    write32be(loc, word | (val & 0xffff));
    break;
  }
  case R_AVR32_32_CPENT:
    write32be(loc, val);
    break;
  case R_AVR32_8_PCREL:
    checkInt(ctx, loc, val, 8, rel);
    *loc = val;
    break;
  case R_AVR32_16_PCREL:
    checkInt(ctx, loc, val, 16, rel);
    write16be(loc, val);
    break;
  case R_AVR32_32_PCREL:
    checkInt(ctx, loc, val, 32, rel);
    write32be(loc, val);
    break;
  case R_AVR32_22H_PCREL: {
    int64_t pcrel = getAlignedPCRel(rel, val, 2);
    checkAlignment(ctx, loc, pcrel, 2, rel);
    int64_t disp = pcrel >> 1;
    checkInt(ctx, loc, disp, 21, rel);
    uint32_t word = read32be(loc) & ~0x1e10ffff;
    write32be(loc, word | encodeS21(disp));
    break;
  }
  case R_AVR32_11H_PCREL: {
    int64_t pcrel = getAlignedPCRel(rel, val, 2);
    checkAlignment(ctx, loc, pcrel, 2, rel);
    int64_t disp = pcrel >> 1;
    checkInt(ctx, loc, disp, 10, rel);
    uint16_t word = read16be(loc) & ~0x0ff3;
    uint16_t enc = static_cast<uint16_t>(disp);
    write16be(loc, word | ((enc & 0xff) << 4) | ((enc & 0x300) >> 8));
    break;
  }
  case R_AVR32_9H_PCREL: {
    int64_t pcrel = getAlignedPCRel(rel, val, 2);
    checkAlignment(ctx, loc, pcrel, 2, rel);
    int64_t disp = pcrel >> 1;
    checkInt(ctx, loc, disp, 8, rel);
    uint16_t word = read16be(loc) & ~0x0ff0;
    uint16_t enc = static_cast<uint16_t>(disp);
    write16be(loc, word | ((enc & 0xff) << 4));
    break;
  }
  case R_AVR32_CPCALL: {
    int64_t pcrel = getAlignedPCRel(rel, val, 4);
    checkAlignment(ctx, loc, pcrel, 4, rel);
    int64_t disp = pcrel >> 2;
    checkInt(ctx, loc, disp, 16, rel);
    uint32_t word = read32be(loc) & ~0xffff;
    write32be(loc, word | (disp & 0xffff));
    break;
  }
  case R_AVR32_16_CP: {
    checkInt(ctx, loc, val, 16, rel);
    uint32_t word = read32be(loc) & ~0xffff;
    write32be(loc, word | (val & 0xffff));
    break;
  }
  case R_AVR32_9W_CP: {
    int64_t pcrel = getAlignedPCRel(rel, val, 4);
    checkAlignment(ctx, loc, pcrel, 4, rel);
    int64_t disp = pcrel >> 2;
    checkUInt(ctx, loc, disp, 7, rel);
    uint16_t word = read16be(loc) & ~0x07f0;
    write16be(loc, word | ((static_cast<uint16_t>(disp) & 0x7f) << 4));
    break;
  }
  default:
    llvm_unreachable("unknown relocation");
  }
}

void elf::setAVR32TargetInfo(Ctx &ctx) { ctx.target.reset(new AVR32(ctx)); }
