//===- AVR32.cpp ----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "OutputSections.h"
#include "Symbols.h"
#include "Target.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/TimeProfiler.h"
#include <algorithm>
#include <cstring>

using namespace llvm;
using namespace llvm::ELF;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace lld;
using namespace lld::elf;

static constexpr uint32_t INTERNAL_R_AVR32_RELAX_CALL = UINT32_MAX;
static constexpr uint32_t INTERNAL_R_AVR32_RELAX_LDDPC_MOV = UINT32_MAX - 1;

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
  bool relaxOnce(int pass) const override;
  void finalizeRelax(int passes) const override;
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

static bool canRelaxControlTransfer(Ctx &ctx) {
  return ctx.arg.relax && (ctx.arg.eflags & EF_AVR32_LINKRELAX);
}

static bool canRelaxDirectData(Ctx &ctx) {
  return canRelaxControlTransfer(ctx) && ctx.arg.directData;
}

static bool isFullBranch(uint32_t word) {
  return (word & 0xe1e00000) == 0xe0800000;
}

static bool isFullCall(uint32_t word) {
  return (word & 0xe1ef0000) == 0xe0a00000;
}

static bool isFullControlTransfer(uint32_t word) {
  return isFullBranch(word) || isFullCall(word);
}

static bool isMcallPC(uint32_t word) {
  return (word & 0xffff0000) == 0xf01f0000;
}

static bool isCompactLddpc(uint16_t word) { return (word & 0xf800) == 0x4800; }

static std::optional<uint32_t> findUniqueMcallToCPEntry(ArrayRef<uint8_t> data,
                                                        uint64_t cpOffset) {
  std::optional<uint32_t> found;
  for (uint64_t offset = 0, size = std::min<uint64_t>(data.size(), cpOffset);
       offset + 4 <= size;
       offset += 2) {
    uint32_t word = read32be(data.data() + offset);
    if (!isMcallPC(word))
      continue;

    int64_t pcrel = SignExtend64<16>(word & 0xffff) << 2;
    int64_t target = static_cast<int64_t>(offset & ~uint64_t(3)) + pcrel;
    if (target != static_cast<int64_t>(cpOffset))
      continue;
    if (found)
      return std::nullopt;
    found = offset;
  }
  return found;
}

static std::optional<uint32_t> findUniqueLddpcToCPEntry(ArrayRef<uint8_t> data,
                                                        uint64_t cpOffset) {
  std::optional<uint32_t> found;
  for (uint64_t offset = 0, size = std::min<uint64_t>(data.size(), cpOffset);
       offset + 2 <= size; offset += 2) {
    uint16_t word = read16be(data.data() + offset);
    if (!isCompactLddpc(word))
      continue;

    int64_t pcrel = ((word & 0x07f0) >> 4) << 2;
    int64_t target = static_cast<int64_t>(offset & ~uint64_t(3)) + pcrel;
    if (target != static_cast<int64_t>(cpOffset))
      continue;
    if (found)
      return std::nullopt;
    found = offset;
  }
  return found;
}

static uint32_t getMovri21Base(uint16_t lddpc) {
  uint32_t rd = lddpc & 0xf;
  return 0xe0600000 | (rd << 16);
}

static bool hasSymbolAnchorInRange(ArrayRef<SymbolAnchor> anchors,
                                   uint64_t begin, uint64_t end) {
  for (const SymbolAnchor &a : anchors)
    if (begin <= a.offset && a.offset < end)
      return true;
  return false;
}

static uint32_t getDeltaBefore(ArrayRef<Relocation> rels, const RelaxAux &aux,
                               uint64_t offset) {
  uint32_t delta = 0;
  for (size_t i = 0, e = rels.size(); i != e && rels[i].offset < offset; ++i)
    delta = aux.relocDeltas[i];
  return delta;
}

static uint64_t getRelaxedOffset(ArrayRef<Relocation> rels, const RelaxAux &aux,
                                 uint64_t offset) {
  return offset - getDeltaBefore(rels, aux, offset);
}

static std::optional<std::pair<RelType, uint16_t>>
getRelaxedControlTransfer(uint32_t word, int64_t pcrel) {
  if (pcrel & 1)
    return std::nullopt;

  int64_t disp = pcrel >> 1;
  if (isFullCall(word)) {
    if (isInt<10>(disp))
      return std::make_pair(R_AVR32_11H_PCREL, static_cast<uint16_t>(0xc00c));
    return std::nullopt;
  }

  if (!isFullBranch(word))
    return std::nullopt;

  uint32_t cond = (word >> 16) & 0xf;
  if (cond == 0xf) {
    if (isInt<10>(disp))
      return std::make_pair(R_AVR32_11H_PCREL, static_cast<uint16_t>(0xc008));
    return std::nullopt;
  }

  if (cond <= 7 && isInt<8>(disp))
    return std::make_pair(R_AVR32_9H_PCREL,
                          static_cast<uint16_t>(0xc000 | cond));

  return std::nullopt;
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
  case R_AVR32_18W_PCREL:
  case R_AVR32_22H_PCREL:
  case R_AVR32_16B_PCREL:
  case R_AVR32_11H_PCREL:
  case R_AVR32_9H_PCREL:
  case R_AVR32_9UW_PCREL:
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
  case R_AVR32_16B_PCREL:
    return SignExtend64<16>(read32be(buf) & 0xffff);
  case R_AVR32_11H_PCREL: {
    uint16_t word = read16be(buf);
    uint16_t enc = ((word >> 4) & 0xff) | ((word & 0x3) << 8);
    return SignExtend64<10>(enc) << 1;
  }
  case R_AVR32_9H_PCREL: {
    uint16_t word = read16be(buf);
    return SignExtend64<8>((word & 0x0ff0) >> 4) << 1;
  }
  case R_AVR32_9UW_PCREL:
    return ((read16be(buf) & 0x07f0) >> 4) << 2;
  case R_AVR32_HI16:
    return (read32be(buf) & 0xffff) << 16;
  case R_AVR32_LO16:
    return read32be(buf) & 0xffff;
  case R_AVR32_18W_PCREL:
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

static bool relax(Ctx &ctx, InputSection &sec) {
  const uint64_t secAddr = sec.getVA();
  const MutableArrayRef<Relocation> relocs = sec.relocs();
  auto &aux = *sec.relaxAux;
  bool changed = false;
  ArrayRef<SymbolAnchor> sa = ArrayRef(aux.anchors);
  uint64_t delta = 0;

  std::fill_n(aux.relocTypes.get(), relocs.size(), R_AVR32_NONE);
  aux.writes.clear();
  for (auto [i, r] : llvm::enumerate(relocs)) {
    const uint64_t loc = secAddr + r.offset - delta;
    uint32_t &cur = aux.relocDeltas[i], remove = 0;
    if (r.type == R_AVR32_22H_PCREL && r.expr == R_PC) {
      uint32_t word = read32be(sec.content().data() + r.offset);
      if (isFullControlTransfer(word)) {
        uint64_t val = sec.getRelocTargetVA(ctx, r, loc);
        int64_t pcrel = getAlignedPCRel(r, SignExtend64(val, 32), 2);
        if (auto relaxed = getRelaxedControlTransfer(word, pcrel)) {
          aux.relocTypes[i] = relaxed->first;
          aux.writes.push_back(relaxed->second);
          remove = 2;
        }
      }
    } else if (canRelaxDirectData(ctx) && r.type == R_AVR32_32_CPENT &&
               !r.sym->isPreemptible && !r.sym->isGnuIFunc()) {
      if (std::optional<uint32_t> callOffset =
              findUniqueMcallToCPEntry(sec.content(), r.offset)) {
        uint64_t callLoc =
            secAddr + *callOffset - getDeltaBefore(relocs, aux, *callOffset);
        uint64_t val = sec.getRelocTargetVA(ctx, r, loc);
        int64_t pcrel = SignExtend64(val - callLoc, 32);
        if (!(pcrel & 1) && isInt<21>(pcrel >> 1)) {
          aux.relocTypes[i] = INTERNAL_R_AVR32_RELAX_CALL;
          aux.writes.push_back(*callOffset);
          remove = 4;
        }
      } else if (std::optional<uint32_t> loadOffset =
                     findUniqueLddpcToCPEntry(sec.content(), r.offset)) {
        // The load is 2 bytes and the pool entry is 4 bytes. Replacing the
        // load plus immediately following pool bytes with a 4-byte mov is
        // layout-local; wider cases would require inserting bytes before
        // arbitrary code, which this relaxation pass does not model.
        uint32_t gap = r.offset - *loadOffset;
        bool poolFollowsLoad =
            gap == 2 ||
            (gap == 4 &&
             read16be(sec.content().data() + *loadOffset + 2) == 0 &&
             !hasSymbolAnchorInRange(aux.anchors, *loadOffset + 2, r.offset));
        uint64_t val = sec.getRelocTargetVA(ctx, r, loc);
        if (poolFollowsLoad && isInt<21>(SignExtend64(val, 32))) {
          aux.relocTypes[i] = INTERNAL_R_AVR32_RELAX_LDDPC_MOV;
          aux.writes.push_back(*loadOffset);
          aux.writes.push_back(
              getMovri21Base(read16be(sec.content().data() + *loadOffset)));
          remove = gap;
        }
      }
    }

    for (; sa.size() && sa[0].offset <= r.offset; sa = sa.slice(1)) {
      if (sa[0].end)
        sa[0].d->size = sa[0].offset - delta - sa[0].d->value;
      else
        sa[0].d->value = sa[0].offset - delta;
    }
    delta += remove;
    if (delta != cur) {
      cur = delta;
      changed = true;
    }
  }

  for (const SymbolAnchor &a : sa) {
    if (a.end)
      a.d->size = a.offset - delta - a.d->value;
    else
      a.d->value = a.offset - delta;
  }
  if (!isUInt<32>(delta))
    Err(ctx) << "section size decrease is too large: " << delta;
  sec.bytesDropped = delta;
  return changed;
}

bool AVR32::relaxOnce(int pass) const {
  if (!canRelaxControlTransfer(ctx))
    return false;

  llvm::TimeTraceScope timeScope("AVR32 relaxOnce");
  if (pass == 0)
    initSymbolAnchors(ctx);

  SmallVector<InputSection *, 0> storage;
  bool changed = false;
  for (OutputSection *osec : ctx.outputSections) {
    if (!(osec->flags & SHF_EXECINSTR))
      continue;
    for (InputSection *sec : getInputSections(*osec, storage))
      if (sec->relaxAux && sec->relaxAux->relocDeltas)
        changed |= relax(ctx, *sec);
  }
  return changed;
}

void AVR32::finalizeRelax(int passes) const {
  if (!canRelaxControlTransfer(ctx))
    return;

  llvm::TimeTraceScope timeScope("Finalize AVR32 relaxation");
  Log(ctx) << "relaxation passes: " << passes;
  SmallVector<InputSection *, 0> storage;
  for (OutputSection *osec : ctx.outputSections) {
    if (!(osec->flags & SHF_EXECINSTR))
      continue;
    for (InputSection *sec : getInputSections(*osec, storage)) {
      if (!sec->relaxAux || !sec->relaxAux->relocDeltas)
        continue;

      RelaxAux &aux = *sec->relaxAux;
      MutableArrayRef<Relocation> rels = sec->relocs();
      if (aux.relocDeltas[rels.size() - 1] == 0 && aux.writes.empty())
        continue;

      ArrayRef<uint8_t> old = sec->content();
      size_t newSize = old.size() - aux.relocDeltas[rels.size() - 1];
      SmallVector<uint32_t, 0> relaxedCallOffsets(rels.size(), UINT32_MAX);
      SmallVector<uint32_t, 0> relaxedLoadOffsets(rels.size(), UINT32_MAX);
      size_t writesScanIdx = 0;
      for (size_t i = 0, e = rels.size(); i != e; ++i) {
        switch (aux.relocTypes[i].v) {
        case R_AVR32_11H_PCREL:
        case R_AVR32_9H_PCREL:
          ++writesScanIdx;
          break;
        case INTERNAL_R_AVR32_RELAX_CALL:
          relaxedCallOffsets[i] =
              getRelaxedOffset(rels, aux, aux.writes[writesScanIdx++]);
          break;
        case INTERNAL_R_AVR32_RELAX_LDDPC_MOV:
          relaxedLoadOffsets[i] =
              getRelaxedOffset(rels, aux, aux.writes[writesScanIdx]);
          writesScanIdx += 2;
          break;
        default:
          break;
        }
      }
      size_t writesIdx = 0;
      uint8_t *newContent = ctx.bAlloc.Allocate<uint8_t>(newSize);
      uint8_t *p = newContent;
      uint64_t offset = 0;
      int64_t delta = 0;
      sec->content_ = newContent;
      sec->size = newSize;
      sec->bytesDropped = 0;

      for (size_t i = 0, e = rels.size(); i != e; ++i) {
        uint32_t remove = aux.relocDeltas[i] - delta;
        delta = aux.relocDeltas[i];
        if (remove == 0 && aux.relocTypes[i] == R_AVR32_NONE)
          continue;

        const Relocation &r = rels[i];
        if (aux.relocTypes[i] == INTERNAL_R_AVR32_RELAX_LDDPC_MOV) {
          uint32_t loadOffset = aux.writes[writesIdx++];
          uint32_t mov = aux.writes[writesIdx++];
          assert(loadOffset >= offset && "overlapping AVR32 relaxation");
          uint64_t size = loadOffset - offset;
          memcpy(p, old.data() + offset, size);
          p += size;
          write32be(p, mov);
          p += 4;
          offset = r.offset + 4;
          continue;
        }

        uint64_t size = r.offset - offset;
        memcpy(p, old.data() + offset, size);
        p += size;

        int64_t skip = 0;
        if (RelType newType = aux.relocTypes[i]) {
          switch (newType.v) {
          case R_AVR32_11H_PCREL:
          case R_AVR32_9H_PCREL:
            skip = 2;
            write16be(p, aux.writes[writesIdx++]);
            break;
          case INTERNAL_R_AVR32_RELAX_CALL:
            ++writesIdx;
            break;
          default:
            llvm_unreachable("unsupported type");
          }
        }

        p += skip;
        offset = r.offset + skip + remove;
      }
      memcpy(p, old.data() + offset, old.size() - offset);

      for (uint32_t callOffset : relaxedCallOffsets)
        if (callOffset != UINT32_MAX)
          write32be(newContent + callOffset, 0xe0a00000);

      delta = 0;
      for (size_t i = 0, e = rels.size(); i != e;) {
        uint64_t cur = rels[i].offset;
        do {
          if (relaxedCallOffsets[i] != UINT32_MAX) {
            rels[i].expr = R_PC;
            rels[i].offset = relaxedCallOffsets[i];
            rels[i].type = R_AVR32_22H_PCREL;
          } else if (relaxedLoadOffsets[i] != UINT32_MAX) {
            rels[i].expr = R_ABS;
            rels[i].offset = relaxedLoadOffsets[i];
            rels[i].type = R_AVR32_21S;
          } else {
            rels[i].offset -= delta;
            if (aux.relocTypes[i] != R_AVR32_NONE)
              rels[i].type = aux.relocTypes[i];
          }
        } while (++i != e && rels[i].offset == cur);
        delta = aux.relocDeltas[i - 1];
      }
    }
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
  case R_AVR32_16B_PCREL: {
    checkInt(ctx, loc, val, 16, rel);
    uint32_t word = read32be(loc) & ~0xffff;
    write32be(loc, word | (val & 0xffff));
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
  case R_AVR32_9UW_PCREL: {
    int64_t pcrel = getAlignedPCRel(rel, val, 4);
    checkAlignment(ctx, loc, pcrel, 4, rel);
    int64_t disp = pcrel >> 2;
    checkUInt(ctx, loc, disp, 7, rel);
    uint16_t word = read16be(loc) & ~0x07f0;
    write16be(loc, word | ((static_cast<uint16_t>(disp) & 0x7f) << 4));
    break;
  }
  case R_AVR32_18W_PCREL:
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
