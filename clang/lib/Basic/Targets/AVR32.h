//===--- AVR32.h - Declare AVR32 target feature support ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_AVR32_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_AVR32_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY AVR32TargetInfo : public TargetInfo {
  static const TargetInfo::GCCRegAlias GCCRegAliases[];
  static const char *const GCCRegNames[];

public:
  AVR32TargetInfo(const llvm::Triple &Triple, const TargetOptions &)
      : TargetInfo(Triple) {
    TLSSupported = false;
    WIntType = UnsignedInt;
    LongLongAlign = DoubleAlign = LongDoubleAlign = SuitableAlign = 32;
    resetDataLayout("E-m:e-p:32:32-i64:32-f64:32-n32-S32");
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override;

  bool hasFeature(StringRef Feature) const override {
    return Feature == "avr32";
  }

  bool allowsLargerPreferedTypeAlignment() const override { return false; }

  bool isValidCPUName(StringRef Name) const override;
  void fillValidCPUList(SmallVectorImpl<StringRef> &Values) const override;
  bool setCPU(const std::string &Name) override;

  ArrayRef<const char *> getGCCRegNames() const override;
  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override;

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    switch (*Name) {
    default:
      return false;
    case 'r':
      Info.setAllowsRegister();
      return true;
    case 'K':
      if (Name[1] == 's' && Name[2] == '2' && Name[3] == '1') {
        Info.setRequiresImmediate(-1048576, 1048575);
        Name += 3;
        return true;
      }
      return false;
    }
  }

  std::string convertConstraint(const char *&Constraint) const override {
    if (Constraint[0] == 'K' && Constraint[1] == 's' &&
        Constraint[2] == '2' && Constraint[3] == '1') {
      std::string Result = std::string("@4") + std::string(Constraint, 4);
      Constraint += 3;
      return Result;
    }

    return TargetInfo::convertConstraint(Constraint);
  }

  std::string_view getClobbers() const override { return ""; }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::CharPtrBuiltinVaList;
  }

protected:
  std::string CPU;
  const char *DefineName = nullptr;
  bool IsUC = false;
  bool IsAP = false;
  bool HasNoMul = false;
};

} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_AVR32_H
