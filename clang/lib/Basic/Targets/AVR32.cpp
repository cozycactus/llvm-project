//===--- AVR32.cpp - Implement AVR32 target feature support ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32.h"
#include "clang/Basic/MacroBuilder.h"

using namespace clang;
using namespace clang::targets;

const char *const AVR32TargetInfo::GCCRegNames[] = {
    "r0",  "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8",  "r9", "r10", "r11", "r12", "sp", "lr", "pc",
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
}
