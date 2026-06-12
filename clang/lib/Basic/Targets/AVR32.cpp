//===--- AVR32.cpp - Implement AVR32 target feature support ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32.h"
#include "clang/Basic/MacroBuilder.h"
#include "clang/Basic/TargetBuiltins.h"

using namespace clang;
using namespace clang::targets;

static constexpr int NumBuiltins =
    clang::AVR32::LastTSBuiltin - Builtin::FirstTSBuiltin;

static constexpr llvm::StringTable BuiltinStrings =
    CLANG_BUILTIN_STR_TABLE_START
#define BUILTIN CLANG_BUILTIN_STR_TABLE
#include "clang/Basic/BuiltinsAVR32.def"
#undef BUILTIN
    ;

static constexpr auto BuiltinInfos = Builtin::MakeInfos<NumBuiltins>({
#define BUILTIN CLANG_BUILTIN_ENTRY
#define LIBBUILTIN CLANG_LIBBUILTIN_ENTRY
#include "clang/Basic/BuiltinsAVR32.def"
#undef BUILTIN
#undef LIBBUILTIN
});

namespace {

struct AVR32CPUInfo {
  const char *Name;
  const char *DefineName;
  bool IsUC;
  bool IsAP;
  bool HasNoMul;
};

static constexpr AVR32CPUInfo AVR32CPUs[] = {
    {"generic", nullptr, false, false, false},
    {"ap", "__AVR32_AP__", false, true, false},
    {"uc3a3128", "__AVR32_UC3A3128__", true, false, false},
    {"uc3a3256", "__AVR32_UC3A3256__", true, false, false},
    {"uc3a3256s", "__AVR32_UC3A3256S__", true, false, false},
    {"uc3a3revd", "__AVR32_UC3A3256S__", true, false, true},
};

const AVR32CPUInfo *getAVR32CPUInfo(StringRef Name) {
  return llvm::find_if(AVR32CPUs, [Name](const AVR32CPUInfo &Info) {
    return Info.Name == Name;
  });
}

} // namespace

const char *const AVR32TargetInfo::GCCRegNames[] = {
    "r0", "r1", "r2",  "r3",  "r4",  "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc",
};

const TargetInfo::GCCRegAlias AVR32TargetInfo::GCCRegAliases[] = {
    {{"r13"}, "sp"},
    {{"r14"}, "lr"},
    {{"r15"}, "pc"},
};

ArrayRef<const char *> AVR32TargetInfo::getGCCRegNames() const {
  return llvm::ArrayRef(GCCRegNames);
}

ArrayRef<TargetInfo::GCCRegAlias> AVR32TargetInfo::getGCCRegAliases() const {
  return llvm::ArrayRef(GCCRegAliases);
}

void AVR32TargetInfo::getTargetDefines(const LangOptions &Opts,
                                       MacroBuilder &Builder) const {
  Builder.defineMacro("__AVR32__");
  Builder.defineMacro("__avr32__");
  Builder.defineMacro("__AVR32_ELF__");
  Builder.defineMacro("__REGISTER_PREFIX__", "");

  if (IsUC) {
    Builder.defineMacro("__AVR32_AVR32A__");
    Builder.defineMacro("__AVR32_UC__", "2");
    Builder.defineMacro("__AVR32_HAS_DSP__");
    Builder.defineMacro("__AVR32_HAS_RMW__");
  }
  if (IsAP) {
    Builder.defineMacro("__AVR32_AVR32B__");
    Builder.defineMacro("__AVR32_HAS_BRANCH_PRED__");
    Builder.defineMacro("__AVR32_HAS_DSP__");
    Builder.defineMacro("__AVR32_HAS_SIMD__");
    Builder.defineMacro("__AVR32_HAS_UNALIGNED_WORD__");
  }

  if (DefineName)
    Builder.defineMacro(DefineName);
  if (HasNoMul)
    Builder.defineMacro("__AVR32_NO_MUL__");
}

llvm::SmallVector<Builtin::InfosShard>
AVR32TargetInfo::getTargetBuiltins() const {
  return {{&BuiltinStrings, BuiltinInfos}};
}

bool AVR32TargetInfo::isValidCPUName(StringRef Name) const {
  return getAVR32CPUInfo(Name) != std::end(AVR32CPUs);
}

void AVR32TargetInfo::fillValidCPUList(
    SmallVectorImpl<StringRef> &Values) const {
  for (const AVR32CPUInfo &Info : AVR32CPUs)
    Values.push_back(Info.Name);
}

bool AVR32TargetInfo::setCPU(const std::string &Name) {
  const AVR32CPUInfo *Info = getAVR32CPUInfo(Name);
  if (Info == std::end(AVR32CPUs))
    return false;

  CPU = Name;
  DefineName = Info->DefineName;
  IsUC = Info->IsUC;
  IsAP = Info->IsAP;
  HasNoMul = Info->HasNoMul;
  return true;
}
