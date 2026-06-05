//===-- AVR32MCAsmInfo.h - AVR32 Asm Info ----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR32_MCTARGETDESC_AVR32MCASMINFO_H
#define LLVM_LIB_TARGET_AVR32_MCTARGETDESC_AVR32MCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {

class MCSpecifierExpr;
class Triple;

class AVR32MCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit AVR32MCAsmInfo(const MCTargetOptions &Options);
  void printSpecifierExpr(raw_ostream &OS,
                          const MCSpecifierExpr &Expr) const override;
};

} // end namespace llvm

#endif
