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
#include "llvm/ADT/DenseMap.h"
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
static constexpr uint32_t INTERNAL_R_AVR32_RELAX_FULL_LDDPC_MOV =
    UINT32_MAX - 2;
static constexpr uint32_t INTERNAL_R_AVR32_RELAX_SKIP_RELOC = UINT32_MAX - 3;

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
  void relocateAlloc(InputSection &sec, uint8_t *buf) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};
} // namespace

static uint32_t encodeS21(int64_t val) {
  uint32_t enc = static_cast<uint32_t>(val);
  return (enc & 0xffff) | ((enc & 0x10000) << 4) | ((enc & 0x1e0000) << 8);
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
  return ctx.arg.relax;
}

static bool canRelaxDirectData(Ctx &ctx) {
  return canRelaxControlTransfer(ctx) && ctx.arg.directData;
}

static bool canRemoveHalfwords(Ctx &ctx) {
  return ctx.arg.eflags & EF_AVR32_LINKRELAX;
}

static bool isLinkRelaxableSection(const InputSection &sec) {
  return isa_and_nonnull<ObjFile<ELF32BE>>(sec.file) &&
         (getEFlags(sec.file) & EF_AVR32_LINKRELAX);
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

static bool isFullLddpc(uint32_t word) {
  return (word & 0xfff00000) == 0xfef00000;
}

static bool isCompactLddpc(uint16_t word) { return (word & 0xf800) == 0x4800; }

static bool isSameSectionSymbol(const Relocation &r, const InputSection &sec) {
  auto *d = dyn_cast_or_null<Defined>(r.sym);
  return d && d->isSection() && d->section == &sec;
}

static std::optional<uint64_t>
getSameSectionSymbolOffset(const Relocation &r, const InputSection &sec,
                           uint64_t oldSize);

static constexpr size_t AmbiguousMcallRelocIndex = static_cast<size_t>(-1);

static std::optional<uint32_t> findUniqueMcallToCPEntry(ArrayRef<uint8_t> data,
                                                        uint64_t cpOffset) {
  std::optional<uint32_t> found;
  for (uint64_t offset = 0, size = std::min<uint64_t>(data.size(), cpOffset);
       offset + 4 <= size; offset += 2) {
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

static DenseMap<uint64_t, size_t> getMcallRelocsByCPEntry(
    ArrayRef<uint8_t> data, ArrayRef<Relocation> rels, const InputSection &sec,
    uint64_t oldSize) {
  DenseMap<uint64_t, size_t> result;
  for (auto [i, r] : llvm::enumerate(rels)) {
    if (r.type != R_AVR32_18W_PCREL && r.type != R_AVR32_CPCALL)
      continue;
    if (r.offset + 4 > data.size() ||
        !isMcallPC(read32be(data.data() + r.offset)))
      continue;

    std::optional<uint64_t> targetOffset =
        getSameSectionSymbolOffset(r, sec, oldSize);
    if (!targetOffset)
      continue;

    auto [it, inserted] = result.try_emplace(*targetOffset, i);
    if (!inserted)
      it->second = AmbiguousMcallRelocIndex;
  }
  return result;
}

static std::optional<uint32_t> findUniqueLddpcToCPEntry(ArrayRef<uint8_t> data,
                                                        uint64_t cpOffset) {
  std::optional<uint32_t> found;
  for (uint64_t offset = 0, size = std::min<uint64_t>(data.size(), cpOffset);
       offset + 2 <= size; offset += 2) {
    if (offset + 4 <= data.size() &&
        isFullLddpc(read32be(data.data() + offset)))
      continue;

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

static std::optional<uint32_t>
findUniqueFullLddpcToCPEntry(ArrayRef<uint8_t> data, uint64_t cpOffset) {
  std::optional<uint32_t> found;
  for (uint64_t offset = 0, size = std::min<uint64_t>(data.size(), cpOffset);
       offset + 4 <= size; offset += 2) {
    uint32_t word = read32be(data.data() + offset);
    if (!isFullLddpc(word))
      continue;

    int64_t pcrel = SignExtend64<16>(word & 0xffff);
    int64_t target = static_cast<int64_t>(offset) + pcrel;
    if (target != static_cast<int64_t>(cpOffset))
      continue;
    if (found)
      return std::nullopt;
    found = offset;
  }
  return found;
}

static std::optional<size_t> findUniqueFullLddpcRelocIndexToCPEntry(
    ArrayRef<uint8_t> data, ArrayRef<Relocation> rels, const InputSection &sec,
    uint64_t cpOffset) {
  std::optional<size_t> found;
  for (auto [i, r] : llvm::enumerate(rels)) {
    if (r.type != R_AVR32_16B_PCREL || r.expr != R_PC ||
        !isSameSectionSymbol(r, sec) ||
        r.addend != static_cast<int64_t>(cpOffset))
      continue;
    if (r.offset + 4 > data.size() ||
        !isFullLddpc(read32be(data.data() + r.offset)))
      continue;
    if (found)
      return std::nullopt;
    found = i;
  }
  return found;
}

static std::optional<uint32_t>
findUniqueFullLddpcRelocToCPEntry(ArrayRef<uint8_t> data,
                                  ArrayRef<Relocation> rels,
                                  const InputSection &sec, uint64_t cpOffset) {
  if (std::optional<size_t> index =
          findUniqueFullLddpcRelocIndexToCPEntry(data, rels, sec, cpOffset))
    return rels[*index].offset;
  return std::nullopt;
}

static uint32_t getMovri21BaseFromReg(uint32_t rd) {
  return 0xe0600000 | (rd << 16);
}

static uint32_t getMovri21Base(uint16_t lddpc) {
  return getMovri21BaseFromReg(lddpc & 0xf);
}

static uint32_t getMovri21Base(uint32_t lddpc) {
  return getMovri21BaseFromReg((lddpc >> 16) & 0xf);
}

static uint16_t getCompactLddpcBase(uint32_t lddpc) {
  return 0x4800 | ((lddpc >> 16) & 0xf);
}

static bool hasSymbolAnchorInRange(ArrayRef<SymbolAnchor> anchors,
                                   uint64_t begin, uint64_t end) {
  for (const SymbolAnchor &a : anchors)
    if (begin <= a.offset && a.offset < end)
      return true;
  return false;
}

static uint64_t getNextRelocOffset(ArrayRef<Relocation> rels, size_t index,
                                   uint64_t end) {
  for (size_t i = index + 1, e = rels.size(); i != e; ++i)
    if (rels[i].offset > rels[index].offset)
      return rels[i].offset;
  return end;
}

static bool isZeroRange(ArrayRef<uint8_t> data, uint64_t begin, uint64_t end) {
  if (begin > end || end > data.size())
    return false;
  for (uint8_t b : data.slice(begin, end - begin))
    if (b != 0)
      return false;
  return true;
}

static bool hasZeroPaddingAnchorInRange(ArrayRef<uint8_t> data,
                                        ArrayRef<SymbolAnchor> anchors,
                                        uint64_t begin, uint64_t end) {
  if (begin >= data.size())
    return false;
  end = std::min(end, static_cast<uint64_t>(data.size()));
  for (const SymbolAnchor &a : anchors) {
    if (a.end || a.offset <= begin)
      continue;
    if (a.d && a.d->getName().starts_with(".L"))
      continue;
    if (a.offset > end)
      break;
    if (isZeroRange(data, begin, a.offset))
      return true;
  }
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

static uint32_t getTotalDelta(ArrayRef<Relocation> rels, const RelaxAux &aux) {
  if (aux.oldRelocCount)
    return aux.relocDeltas[aux.oldRelocCount - 1];

  uint32_t total = 0;
  for (size_t i = 0, e = rels.size(); i != e; ++i)
    total = std::max(total, aux.relocDeltas[i]);
  return total;
}

static uint32_t getDeltaBeforeFinalized(ArrayRef<Relocation> rels,
                                        const RelaxAux &aux, uint64_t offset) {
  if (aux.oldRelocOffsets && aux.oldRelocCount) {
    uint32_t delta = 0;
    uint32_t bestOffset = 0;
    bool found = false;
    for (size_t i = 0, e = aux.oldRelocCount; i != e; ++i) {
      uint32_t oldOffset = aux.oldRelocOffsets[i];
      if (oldOffset < offset && (!found || bestOffset <= oldOffset)) {
        delta = aux.relocDeltas[i];
        bestOffset = oldOffset;
        found = true;
      }
    }
    return delta;
  }

  uint32_t delta = 0, prevDelta = 0;
  for (size_t i = 0, e = rels.size(); i != e;) {
    uint64_t cur = rels[i].offset;
    size_t j = i + 1;
    for (; j != e && rels[j].offset == cur; ++j)
      ;

    uint64_t oldOffset = cur + prevDelta;
    if (oldOffset >= offset)
      break;
    delta = aux.relocDeltas[j - 1];
    prevDelta = delta;
    i = j;
  }
  return delta;
}

static std::optional<uint64_t>
getRelaxedAnchorVA(const InputSection &target, const RelaxAux &aux,
                   uint64_t oldOffset) {
  for (const SymbolAnchor &a : aux.anchors)
    if (!a.end && a.offset == oldOffset && a.d && a.d->section == &target)
      return target.getVA(a.d->value);
  return std::nullopt;
}

struct RelaxedTarget {
  InputSection *sec;
  uint64_t oldOffset;
  uint64_t unrelaxedVA;
};

static std::optional<RelaxedTarget>
getRelaxedTarget(const Defined &d, uint64_t addend, const InputSection &src) {
  if (auto *target = dyn_cast_or_null<InputSection>(d.section)) {
    if (target == &src)
      return std::nullopt;
    return RelaxedTarget{target, addend, target->getVA(addend)};
  }

  auto *osec = dyn_cast_or_null<OutputSection>(d.section);
  if (!osec)
    return std::nullopt;

  uint64_t oldOutputOffset = d.value + addend;
  uint64_t priorDelta = 0;
  SmallVector<InputSection *, 0> storage;
  for (InputSection *target : getInputSections(*osec, storage)) {
    uint64_t totalDelta = 0;
    if (target->relaxAux && target->relocs().size() &&
        target->relaxAux->relocDeltas)
      totalDelta = getTotalDelta(target->relocs(), *target->relaxAux);

    uint64_t oldStart = target->outSecOff + priorDelta;
    uint64_t oldSize = target->content().size() + totalDelta;
    if (oldStart <= oldOutputOffset && oldOutputOffset <= oldStart + oldSize) {
      if (target == &src)
        return std::nullopt;
      return RelaxedTarget{target, oldOutputOffset - oldStart,
                           osec->addr + oldOutputOffset};
    }
    priorDelta += totalDelta;
  }

  return std::nullopt;
}

static std::optional<uint64_t>
getSameSectionSymbolOffset(const Relocation &r, const InputSection &sec,
                           uint64_t oldSize) {
  if (!isSameSectionSymbol(r, sec) || r.addend < 0 ||
      static_cast<uint64_t>(r.addend) > oldSize)
    return std::nullopt;
  return static_cast<uint64_t>(r.addend);
}

static uint64_t getRelocTargetVA(Ctx &ctx, const InputSection &sec,
                                 ArrayRef<Relocation> rels, const RelaxAux &aux,
                                 const Relocation &r, uint64_t loc) {
  if (std::optional<uint64_t> offset =
          getSameSectionSymbolOffset(r, sec, sec.content().size())) {
    uint64_t va = sec.getVA() + getRelaxedOffset(rels, aux, *offset);
    if (r.expr == R_PC)
      return va - loc;
    if (r.expr == R_ABS)
      return va;
  }
  return sec.getRelocTargetVA(ctx, r, loc);
}

static void adjustSameSectionSymbolAddend(const InputSection &sec,
                                          ArrayRef<Relocation> rels,
                                          const RelaxAux &aux, uint64_t oldSize,
                                          Relocation &r) {
  std::optional<uint64_t> offset = getSameSectionSymbolOffset(r, sec, oldSize);
  if (!offset)
    return;
  r.addend -= getDeltaBefore(rels, aux, *offset);
}

static uint64_t adjustCrossSectionRelaxedTarget(const InputSection &src,
                                                const Relocation &r,
                                                uint64_t val) {
  auto *d = dyn_cast<Defined>(r.sym);
  if (!d || !d->isSection() || r.addend < 0)
    return val;

  std::optional<RelaxedTarget> rt =
      getRelaxedTarget(*d, r.addend, src);
  if (!rt || !rt->sec->relaxAux)
    return val;

  RelaxAux &aux = *rt->sec->relaxAux;
  ArrayRef<Relocation> rels = rt->sec->relocs();
  if (!aux.relocDeltas || (!aux.oldRelocCount && rels.empty()))
    return val;

  uint64_t totalDelta = getTotalDelta(rels, aux);
  uint64_t oldSize = rt->sec->content().size() + totalDelta;
  if (rt->oldOffset > oldSize)
    return val;

  if (std::optional<uint64_t> va =
          getRelaxedAnchorVA(*rt->sec, aux, rt->oldOffset))
    return val - (rt->unrelaxedVA - *va);

  uint64_t relaxedVA =
      rt->sec->getVA(rt->oldOffset - getDeltaBeforeFinalized(rels, aux,
                                                             rt->oldOffset));
  return val - (rt->unrelaxedVA - relaxedVA);
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

static std::optional<uint16_t> getRelaxedFullLddpc(uint32_t word,
                                                   int64_t pcrel) {
  if (!isFullLddpc(word) || (pcrel & 3))
    return std::nullopt;

  int64_t disp = pcrel >> 2;
  if (!isUInt<7>(disp))
    return std::nullopt;
  return getCompactLddpcBase(word);
}

static std::optional<size_t> findRelocAtOffset(ArrayRef<Relocation> rels,
                                               uint64_t offset, RelType type) {
  for (auto [i, r] : llvm::enumerate(rels))
    if (r.offset == offset && r.type == type)
      return i;
  return std::nullopt;
}

static bool canRelaxFullLddpcToMov(Ctx &ctx, const InputSection &sec,
                                   ArrayRef<Relocation> rels,
                                   const RelaxAux &aux, size_t cpIndex,
                                   uint32_t loadOffset) {
  const Relocation &cp = rels[cpIndex];
  if (!canRelaxDirectData(ctx) || cp.type != R_AVR32_32_CPENT ||
      cp.sym->isPreemptible || cp.sym->isGnuIFunc())
    return false;

  std::optional<uint32_t> found =
      findUniqueFullLddpcRelocToCPEntry(sec.content(), rels, sec, cp.offset);
  if (!found)
    found = findUniqueFullLddpcToCPEntry(sec.content(), cp.offset);
  if (!found || *found != loadOffset)
    return false;

  uint64_t loc = sec.getVA() + cp.offset - getDeltaBefore(rels, aux, cp.offset);
  uint64_t val = getRelocTargetVA(ctx, sec, rels, aux, cp, loc);
  return isInt<21>(SignExtend64(val, 32));
}

static uint64_t getConstantPoolStart(ArrayRef<uint64_t> cpOffsets,
                                     uint64_t offset) {
  auto it = llvm::lower_bound(cpOffsets, offset);
  assert(it != cpOffsets.end() && *it == offset);
  while (it != cpOffsets.begin() && *(it - 1) + 4 == *it)
    --it;
  return *it;
}

static uint64_t getDirectDataPoolStart(ArrayRef<uint64_t> targetOffsets,
                                       uint64_t offset) {
  auto it = llvm::lower_bound(targetOffsets, offset);
  assert(it != targetOffsets.end() && *it == offset);
  while (it != targetOffsets.begin() && *(it - 1) + 4 == *it)
    --it;
  return *it;
}

static uint64_t getAlignPaddingBytes(ArrayRef<Relocation> rels,
                                     const InputSection &sec, size_t index,
                                     uint64_t align) {
  uint64_t end = sec.content().size();
  for (size_t i = index + 1, e = rels.size(); i != e; ++i) {
    if (rels[i].offset == rels[index].offset && rels[i].type != R_AVR32_ALIGN)
      return 0;
    if (rels[i].offset > rels[index].offset) {
      end = rels[i].offset;
      break;
    }
  }
  uint64_t limit = std::min(end - rels[index].offset, align - 1);
  ArrayRef<uint8_t> data = sec.content().slice(rels[index].offset, limit);
  uint64_t available = 0;
  while (available != data.size() && data[available] == 0)
    ++available;
  return available;
}

static std::optional<uint64_t> getAlignForReloc(Ctx &ctx,
                                                const Relocation &r) {
  if (r.addend < 0 || r.addend >= 63) {
    Err(ctx) << "invalid alignment order for " << r.type << ": " << r.addend;
    return std::nullopt;
  }
  return 1ULL << r.addend;
}

static bool isWordPCRel(RelType type) {
  switch (type) {
  case R_AVR32_18W_PCREL:
  case R_AVR32_9UW_PCREL:
  case R_AVR32_CPCALL:
  case R_AVR32_9W_CP:
    return true;
  default:
    return false;
  }
}

static bool hasWordPCRel(ArrayRef<Relocation> rels) {
  return llvm::any_of(rels,
                      [](const Relocation &r) { return isWordPCRel(r.type); });
}

static bool crossesSameSectionWordPCRel(ArrayRef<Relocation> rels,
                                        const InputSection &sec, size_t index) {
  uint64_t offset = rels[index].offset;
  for (const Relocation &r : rels) {
    if (!isWordPCRel(r.type) || r.type == R_AVR32_CPCALL)
      continue;
    std::optional<uint64_t> targetOffset =
        getSameSectionSymbolOffset(r, sec, sec.content().size());
    if (!targetOffset)
      continue;
    if ((r.offset < offset && offset < *targetOffset) ||
        (*targetOffset < offset && offset < r.offset))
      return true;
  }
  return false;
}

static SmallVector<char, 0>
getPairedHalfwordRemovals(ArrayRef<Relocation> rels,
                          ArrayRef<char> halfwordRemovals) {
  SmallVector<char, 0> paired(rels.size(), false);
  std::optional<size_t> pending;
  for (auto [i, r] : llvm::enumerate(rels)) {
    if (isWordPCRel(r.type))
      pending.reset();
    if (!halfwordRemovals[i])
      continue;
    if (!pending) {
      pending = i;
      continue;
    }
    paired[*pending] = true;
    paired[i] = true;
    pending.reset();
  }
  return paired;
}

static bool canRemoveHalfword(Ctx &ctx, const InputSection &sec,
                              ArrayRef<Relocation> rels, size_t index,
                              uint64_t delta, bool requireWordAlignment) {
  if (requireWordAlignment && crossesSameSectionWordPCRel(rels, sec, index))
    return false;

  for (size_t i = index + 1, e = rels.size(); i != e; ++i) {
    const Relocation &r = rels[i];
    if (r.type != R_AVR32_ALIGN)
      continue;

    std::optional<uint64_t> align = getAlignForReloc(ctx, r);
    if (!align)
      return false;
    if (*align < 4)
      continue;
    if (requireWordAlignment) {
      for (size_t j = index + 1; j != i; ++j) {
        if (!isWordPCRel(rels[j].type))
          continue;
        if (rels[j].type == R_AVR32_CPCALL)
          continue;
        std::optional<uint64_t> targetOffset =
            getSameSectionSymbolOffset(rels[j], sec, sec.content().size());
        if (!targetOffset || *targetOffset < r.offset)
          return false;
      }
    }
    uint64_t loc = sec.getVA() + r.offset - delta - 2;
    uint64_t needed = alignTo(loc, *align) - loc;
    return needed <= getAlignPaddingBytes(rels, sec, i, *align);
  }
  return !requireWordAlignment;
}

static SmallVector<char, 0>
getRelaxableFullLddpcRelocs(Ctx &ctx, const InputSection &sec,
                            ArrayRef<Relocation> rels, const RelaxAux &aux) {
  SmallVector<char, 0> relaxable(rels.size(), false);
  SmallVector<uint64_t, 0> cpOffsets;
  for (const Relocation &r : rels)
    if (r.type == R_AVR32_32_CPENT)
      cpOffsets.push_back(r.offset);
  llvm::sort(cpOffsets);

  SmallVector<uint64_t, 0> directDataTargetOffsets;
  for (const Relocation &r : rels) {
    if (r.type != R_AVR32_16B_PCREL || r.expr != R_PC ||
        r.offset + 4 > sec.content().size())
      continue;
    if (!isFullLddpc(read32be(sec.content().data() + r.offset)))
      continue;

    std::optional<uint64_t> targetOffset =
        getSameSectionSymbolOffset(r, sec, sec.content().size());
    if (!targetOffset)
      continue;

    auto cpIt = llvm::lower_bound(cpOffsets, *targetOffset);
    if (cpIt == cpOffsets.end() || *cpIt != *targetOffset)
      directDataTargetOffsets.push_back(*targetOffset);
  }
  llvm::sort(directDataTargetOffsets);
  directDataTargetOffsets.erase(llvm::unique(directDataTargetOffsets),
                                directDataTargetOffsets.end());

  DenseMap<uint64_t, SmallVector<size_t, 0>> sameSectionGroups;
  SmallVector<std::optional<uint64_t>, 0> compactSameSectionTargets(
      rels.size());
  for (auto [i, r] : llvm::enumerate(rels)) {
    if (r.type != R_AVR32_16B_PCREL || r.expr != R_PC ||
        r.offset + 4 > sec.content().size())
      continue;

    uint32_t word = read32be(sec.content().data() + r.offset);
    if (!isFullLddpc(word))
      continue;

    uint64_t delta = getDeltaBefore(rels, aux, r.offset);
    uint64_t loc = sec.getVA() + r.offset - delta;
    uint64_t val = getRelocTargetVA(ctx, sec, rels, aux, r, loc);
    Relocation relaxedRel = r;
    relaxedRel.offset -= delta;
    int64_t pcrel = getAlignedPCRel(relaxedRel, SignExtend64(val, 32), 4);
    if (!getRelaxedFullLddpc(word, pcrel))
      continue;

    std::optional<uint64_t> targetOffset =
        getSameSectionSymbolOffset(r, sec, sec.content().size());
    if (!targetOffset) {
      relaxable[i] = true;
      continue;
    }
    compactSameSectionTargets[i] = *targetOffset;

    auto cpIt = llvm::lower_bound(cpOffsets, *targetOffset);
    if (cpIt != cpOffsets.end() && *cpIt == *targetOffset) {
      if (std::optional<size_t> cpIndex =
              findRelocAtOffset(rels, *targetOffset, R_AVR32_32_CPENT))
        if (canRelaxFullLddpcToMov(ctx, sec, rels, aux, *cpIndex,
                                   static_cast<uint32_t>(r.offset)))
          continue;
      sameSectionGroups[getConstantPoolStart(cpOffsets, *targetOffset)]
          .push_back(i);
      continue;
    }

    uint64_t groupOffset = *targetOffset;
    if (r.offset < *targetOffset)
      groupOffset = getDirectDataPoolStart(directDataTargetOffsets,
                                           *targetOffset);
    sameSectionGroups[groupOffset].push_back(i);
  }

  for (auto &it : sameSectionGroups) {
    MutableArrayRef<size_t> group = it.second;
    size_t count = group.size() & ~size_t(1);
    for (size_t i = 0; i != count; ++i)
      relaxable[group[i]] = true;
  }

  std::optional<std::pair<size_t, uint64_t>> pending;
  for (auto [i, r] : llvm::enumerate(rels)) {
    if (isWordPCRel(r.type)) {
      pending.reset();
      continue;
    }

    std::optional<uint64_t> targetOffset = compactSameSectionTargets[i];
    if (!targetOffset)
      continue;

    if (pending) {
      // Pair full lddpc relaxations across different literal-pool entries when
      // both targets are after the second load. Then both targets move by the
      // same four bytes, preserving the word-PC alignment rule.
      if (pending->second > r.offset && *targetOffset > r.offset) {
        relaxable[pending->first] = true;
        relaxable[i] = true;
        pending.reset();
        continue;
      }
      pending.reset();
    }
    pending = std::make_pair(i, *targetOffset);
  }
  return relaxable;
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
  case R_AVR32_16N_PCREL:
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
  case R_AVR32_16N_PCREL:
    return -SignExtend64<16>(read32be(buf) & 0xffff);
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

static bool relax(Ctx &ctx, InputSection &sec, int pass) {
  const uint64_t secAddr = sec.getVA();
  const MutableArrayRef<Relocation> relocs = sec.relocs();
  auto &aux = *sec.relaxAux;
  bool changed = false;
  ArrayRef<SymbolAnchor> sa = ArrayRef(aux.anchors);
  uint64_t delta = 0;
  bool hasWordPCRelocs = hasWordPCRel(relocs);
  DenseMap<uint64_t, size_t> mcallRelocsByCPEntry =
      getMcallRelocsByCPEntry(sec.content(), relocs, sec, sec.content().size());
  std::fill_n(aux.relocTypes.get(), relocs.size(), R_AVR32_NONE);
  aux.writes.clear();
  SmallVector<char, 0> relaxableFullLddpc =
      getRelaxableFullLddpcRelocs(ctx, sec, relocs, aux);
  SmallVector<char, 0> pairedHalfwordRemovals =
      getPairedHalfwordRemovals(relocs, relaxableFullLddpc);
  for (auto [i, r] : llvm::enumerate(relocs)) {
    const uint64_t loc = secAddr + r.offset - delta;
    uint32_t &cur = aux.relocDeltas[i], remove = 0;
    size_t oldWritesSize = aux.writes.size();
    if (r.type == R_AVR32_ALIGN) {
      if (std::optional<uint64_t> align = getAlignForReloc(ctx, r)) {
        uint64_t available = getAlignPaddingBytes(relocs, sec, i, *align);
        uint64_t needed = alignTo(loc, *align) - loc;
        if (needed <= available)
          remove = available - needed;
      }
    } else if (r.type == R_AVR32_22H_PCREL && r.expr == R_PC) {
      uint32_t word = read32be(sec.content().data() + r.offset);
      if (isFullControlTransfer(word)) {
        uint64_t val = getRelocTargetVA(ctx, sec, relocs, aux, r, loc);
        Relocation relaxedRel = r;
        relaxedRel.offset -= delta;
        int64_t pcrel = getAlignedPCRel(relaxedRel, SignExtend64(val, 32), 2);
        if (auto relaxed = getRelaxedControlTransfer(word, pcrel)) {
          aux.relocTypes[i] = relaxed->first;
          aux.writes.push_back(relaxed->second);
          remove = 2;
        }
      }
    } else if (relaxableFullLddpc[i]) {
      uint32_t word = read32be(sec.content().data() + r.offset);
      aux.relocTypes[i] = R_AVR32_9W_CP;
      aux.writes.push_back(getCompactLddpcBase(word));
      remove = 2;
    } else if (canRelaxDirectData(ctx) &&
               (r.type == R_AVR32_32 || r.type == R_AVR32_32_CPENT) &&
               !r.sym->isPreemptible && !r.sym->isGnuIFunc()) {
      std::optional<uint32_t> directCallOffset;
      if (r.type == R_AVR32_32_CPENT)
        directCallOffset = findUniqueMcallToCPEntry(sec.content(), r.offset);
      if (directCallOffset) {
        uint64_t callLoc =
            secAddr + *directCallOffset -
            getDeltaBefore(relocs, aux, *directCallOffset);
        uint64_t val = getRelocTargetVA(ctx, sec, relocs, aux, r, loc);
        int64_t pcrel = SignExtend64(val - callLoc, 32);
        if (!(pcrel & 1) && isInt<21>(pcrel >> 1)) {
          aux.relocTypes[i] = INTERNAL_R_AVR32_RELAX_CALL;
          aux.writes.push_back(*directCallOffset);
          remove = 4;
        }
      } else if (auto it = mcallRelocsByCPEntry.find(r.offset);
                 it != mcallRelocsByCPEntry.end() &&
                 it->second != AmbiguousMcallRelocIndex) {
        size_t callIndex = it->second;
        uint32_t callOffset = relocs[callIndex].offset;
        uint64_t callLoc =
            secAddr + callOffset - getDeltaBefore(relocs, aux, callOffset);
        uint64_t val = getRelocTargetVA(ctx, sec, relocs, aux, r, loc);
        int64_t pcrel = SignExtend64(val - callLoc, 32);
        if (!(pcrel & 1) && isInt<21>(pcrel >> 1)) {
          aux.relocTypes[callIndex] = INTERNAL_R_AVR32_RELAX_SKIP_RELOC;
          aux.relocTypes[i] = INTERNAL_R_AVR32_RELAX_CALL;
          aux.writes.push_back(callOffset);
          remove = 4;
        }
      } else if (r.type == R_AVR32_32_CPENT) {
        if (std::optional<size_t> loadIndex =
                findUniqueFullLddpcRelocIndexToCPEntry(sec.content(), relocs,
                                                       sec, r.offset)) {
          // The full load and mov are both 4 bytes, so only the pool entry needs
          // to be deleted. Intervening code can keep its original layout.
          uint64_t val = getRelocTargetVA(ctx, sec, relocs, aux, r, loc);
          if (isInt<21>(SignExtend64(val, 32))) {
            uint32_t loadOffset = relocs[*loadIndex].offset;
            aux.relocTypes[*loadIndex] = INTERNAL_R_AVR32_RELAX_SKIP_RELOC;
            aux.relocTypes[i] = INTERNAL_R_AVR32_RELAX_FULL_LDDPC_MOV;
            aux.writes.push_back(loadOffset);
            aux.writes.push_back(
                getMovri21Base(read32be(sec.content().data() + loadOffset)));
            remove = 4;
          }
        } else if (std::optional<uint32_t> loadOffset =
                       findUniqueFullLddpcToCPEntry(sec.content(), r.offset)) {
          // The full load and mov are both 4 bytes, so only the pool entry needs
          // to be deleted. Intervening code can keep its original layout.
          uint64_t val = getRelocTargetVA(ctx, sec, relocs, aux, r, loc);
          if (isInt<21>(SignExtend64(val, 32))) {
            aux.relocTypes[i] = INTERNAL_R_AVR32_RELAX_FULL_LDDPC_MOV;
            aux.writes.push_back(*loadOffset);
            aux.writes.push_back(
                getMovri21Base(read32be(sec.content().data() + *loadOffset)));
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
          uint64_t val = getRelocTargetVA(ctx, sec, relocs, aux, r, loc);
          if (poolFollowsLoad && isInt<21>(SignExtend64(val, 32))) {
            aux.relocTypes[i] = INTERNAL_R_AVR32_RELAX_LDDPC_MOV;
            aux.writes.push_back(*loadOffset);
            aux.writes.push_back(
                getMovri21Base(read16be(sec.content().data() + *loadOffset)));
            remove = gap;
          }
        }
      }
    }

    // Large 4-byte pool deletions can make late 2-byte branch/load/alignment
    // relaxations toggle around an alignment boundary. After several passes,
    // keep only halfword removals that were already present in the previous
    // pass. Keeping an extra halfword of code or padding is always safe.
    if (remove == 2 && !canRemoveHalfwords(ctx)) {
      aux.relocTypes[i] = R_AVR32_NONE;
      aux.writes.resize(oldWritesSize);
      remove = 0;
    }

    if (remove == 2) {
      uint64_t paddingBegin =
          r.type == R_AVR32_ALIGN ? r.offset : r.offset + 4;
      uint64_t paddingEnd =
          getNextRelocOffset(relocs, i, sec.content().size());
      if (hasZeroPaddingAnchorInRange(sec.content(), aux.anchors, paddingBegin,
                                      paddingEnd)) {
        aux.relocTypes[i] = R_AVR32_NONE;
        aux.writes.resize(oldWritesSize);
        remove = 0;
      }
    }

    if (pass >= 8 && remove == 2 && cur >= delta && cur - delta == 0) {
      aux.relocTypes[i] = R_AVR32_NONE;
      aux.writes.resize(oldWritesSize);
      remove = 0;
    }

    if (r.type != R_AVR32_ALIGN && remove == 2 &&
        !pairedHalfwordRemovals[i] &&
        !canRemoveHalfword(ctx, sec, relocs, i, delta, hasWordPCRelocs)) {
      aux.relocTypes[i] = R_AVR32_NONE;
      aux.writes.resize(oldWritesSize);
      remove = 0;
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
      if (isLinkRelaxableSection(*sec) && sec->relaxAux &&
          sec->relaxAux->relocDeltas)
        changed |= relax(ctx, *sec, pass);
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
      if (!isLinkRelaxableSection(*sec) || !sec->relaxAux ||
          !sec->relaxAux->relocDeltas)
        continue;

      RelaxAux &aux = *sec->relaxAux;
      MutableArrayRef<Relocation> rels = sec->relocs();
      if (aux.relocDeltas[rels.size() - 1] == 0 && aux.writes.empty())
        continue;

      if (!aux.oldRelocOffsets)
        aux.oldRelocOffsets = std::make_unique<uint32_t[]>(rels.size());
      aux.oldRelocCount = rels.size();
      for (auto [i, rel] : llvm::enumerate(rels))
        aux.oldRelocOffsets[i] = rel.offset;

      ArrayRef<uint8_t> old = sec->content();
      size_t newSize = old.size() - aux.relocDeltas[rels.size() - 1];
      SmallVector<uint32_t, 0> relaxedCallOffsets(rels.size(), UINT32_MAX);
      SmallVector<uint32_t, 0> relaxedLoadOffsets(rels.size(), UINT32_MAX);
      SmallVector<uint32_t, 0> relaxedFullLoadOffsets(rels.size(), UINT32_MAX);
      SmallVector<uint32_t, 0> relaxedFullLoadWrites(rels.size());
      size_t writesScanIdx = 0;
      for (size_t i = 0, e = rels.size(); i != e; ++i) {
        switch (aux.relocTypes[i].v) {
        case R_AVR32_11H_PCREL:
        case R_AVR32_9H_PCREL:
        case R_AVR32_9W_CP:
          ++writesScanIdx;
          break;
        case INTERNAL_R_AVR32_RELAX_SKIP_RELOC:
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
        case INTERNAL_R_AVR32_RELAX_FULL_LDDPC_MOV:
          relaxedFullLoadOffsets[i] =
              getRelaxedOffset(rels, aux, aux.writes[writesScanIdx]);
          relaxedFullLoadWrites[i] = aux.writes[writesScanIdx + 1];
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
          case R_AVR32_9W_CP:
            skip = 2;
            write16be(p, aux.writes[writesIdx++]);
            break;
          case INTERNAL_R_AVR32_RELAX_SKIP_RELOC:
            break;
          case INTERNAL_R_AVR32_RELAX_CALL:
            ++writesIdx;
            break;
          case INTERNAL_R_AVR32_RELAX_FULL_LDDPC_MOV:
            writesIdx += 2;
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
      for (size_t i = 0, e = rels.size(); i != e; ++i)
        if (relaxedFullLoadOffsets[i] != UINT32_MAX)
          write32be(newContent + relaxedFullLoadOffsets[i],
                    relaxedFullLoadWrites[i]);

      for (Relocation &rel : rels)
        adjustSameSectionSymbolAddend(*sec, rels, aux, old.size(), rel);

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
          } else if (relaxedFullLoadOffsets[i] != UINT32_MAX) {
            rels[i].expr = R_ABS;
            rels[i].offset = relaxedFullLoadOffsets[i];
            rels[i].type = R_AVR32_21S;
          } else if (aux.relocTypes[i] == INTERNAL_R_AVR32_RELAX_SKIP_RELOC) {
            rels[i].expr = R_NONE;
            rels[i].type = R_AVR32_NONE;
          } else {
            rels[i].offset -= delta;
            if (aux.relocTypes[i] != R_AVR32_NONE)
              rels[i].type = aux.relocTypes[i];
          }
        } while (++i != e && rels[i].offset == cur);
        delta = aux.relocDeltas[i - 1];
      }
      llvm::erase_if(sec->relocations, [](const Relocation &r) {
        return r.expr == R_NONE && r.type == R_AVR32_NONE;
      });
    }
  }
}

void AVR32::relocateAlloc(InputSection &sec, uint8_t *buf) const {
  uint64_t secAddr = sec.getOutputSection()->addr + sec.outSecOff;
  for (const Relocation &rel : sec.relocs()) {
    if (rel.expr == R_NONE)
      continue;
    uint8_t *loc = buf + rel.offset;
    uint64_t val =
        SignExtend64(sec.getRelocTargetVA(ctx, rel, secAddr + rel.offset), 32);
    val = adjustCrossSectionRelaxedTarget(sec, rel, val);
    if (rel.expr != R_RELAX_HINT)
      relocate(loc, rel, val);
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
  case R_AVR32_16N_PCREL: {
    int64_t neg = -val;
    checkInt(ctx, loc, neg, 16, rel);
    uint32_t word = read32be(loc) & ~0xffff;
    write32be(loc, word | (neg & 0xffff));
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
