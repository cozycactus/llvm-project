//===- AVR32.cpp ----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABIInfoImpl.h"
#include "TargetInfo.h"

using namespace clang;
using namespace clang::CodeGen;

namespace {

class AVR32TargetCodeGenInfo : public TargetCodeGenInfo {
public:
  AVR32TargetCodeGenInfo(CodeGenTypes &CGT)
      : TargetCodeGenInfo(std::make_unique<DefaultABIInfo>(CGT)) {}

  void setTargetAttributes(const Decl *D, llvm::GlobalValue *GV,
                           CodeGen::CodeGenModule &CGM) const override {
    if (GV->isDeclaration())
      return;

    const auto *FD = dyn_cast_or_null<FunctionDecl>(D);
    if (!FD)
      return;

    if (FD->getAttr<AVR32InterruptAttr>())
      cast<llvm::Function>(GV)->addFnAttr("interrupt");
  }
};

} // namespace

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createAVR32TargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<AVR32TargetCodeGenInfo>(CGM.getTypes());
}
