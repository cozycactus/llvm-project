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

    const AVR32InterruptAttr *Attr = FD->getAttr<AVR32InterruptAttr>();
    if (!Attr)
      return;

    auto *Fn = cast<llvm::Function>(GV);
    switch (Attr->getInterrupt()) {
    case AVR32InterruptAttr::Default:
    case AVR32InterruptAttr::None:
      Fn->addFnAttr("interrupt");
      break;
    case AVR32InterruptAttr::Half:
      Fn->addFnAttr("interrupt", "half");
      break;
    case AVR32InterruptAttr::Full:
      Fn->addFnAttr("interrupt", "full");
      break;
    }
  }
};

} // namespace

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createAVR32TargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<AVR32TargetCodeGenInfo>(CGM.getTypes());
}
