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
    return R_NONE;
  case R_AVR32_8:
  case R_AVR32_16:
  case R_AVR32_32:
    return R_ABS;
  case R_AVR32_8_PCREL:
  case R_AVR32_16_PCREL:
  case R_AVR32_32_PCREL:
  case R_AVR32_22H_PCREL:
  case R_AVR32_11H_PCREL:
    return R_PC;
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unknown relocation (" << type.v
             << ") against symbol " << &s;
    return R_NONE;
  }
}

int64_t AVR32::getImplicitAddend(const uint8_t *buf, RelType type) const {
  if (type == R_AVR32_NONE)
    return 0;
  return TargetInfo::getImplicitAddend(buf, type);
}

void AVR32::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  switch (rel.type) {
  case R_AVR32_NONE:
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
    checkAlignment(ctx, loc, val, 2, rel);
    int64_t disp = SignExtend64(val, 32) >> 1;
    checkInt(ctx, loc, disp, 21, rel);
    uint32_t word = read32be(loc) & ~0x1e10ffff;
    uint32_t enc = static_cast<uint32_t>(disp);
    write32be(loc, word | (enc & 0xffff) | ((enc & 0x10000) << 4) |
                       ((enc & 0x1e0000) << 8));
    break;
  }
  case R_AVR32_11H_PCREL: {
    checkAlignment(ctx, loc, val, 2, rel);
    int64_t disp = SignExtend64(val, 32) >> 1;
    checkInt(ctx, loc, disp, 10, rel);
    uint16_t word = read16be(loc) & ~0x0ff3;
    uint16_t enc = static_cast<uint16_t>(disp);
    write16be(loc, word | ((enc & 0xff) << 4) | ((enc & 0x300) >> 8));
    break;
  }
  default:
    llvm_unreachable("unknown relocation");
  }
}

void elf::setAVR32TargetInfo(Ctx &ctx) { ctx.target.reset(new AVR32(ctx)); }
