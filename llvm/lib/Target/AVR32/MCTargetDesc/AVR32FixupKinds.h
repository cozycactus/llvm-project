//===-- AVR32FixupKinds.h - AVR32 Specific Fixup Entries -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR32_MCTARGETDESC_AVR32FIXUPKINDS_H
#define LLVM_LIB_TARGET_AVR32_MCTARGETDESC_AVR32FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace AVR32 {

enum Fixups {
  fixup_22h_pcrel = FirstTargetFixupKind,
  fixup_11h_pcrel,
  fixup_21s,

  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};

} // end namespace AVR32
} // end namespace llvm

#endif
