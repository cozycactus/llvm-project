//===-- AVR32SelectionDAGInfo.h - AVR32 SelectionDAG info ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR32_AVR32SELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_AVR32_AVR32SELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"

#define GET_SDNODE_ENUM
#include "AVR32GenSDNodeInfo.inc"

namespace llvm {

class AVR32SelectionDAGInfo : public SelectionDAGGenTargetInfo {
public:
  AVR32SelectionDAGInfo();
};
} // namespace llvm

#endif // LLVM_LIB_TARGET_AVR32_AVR32SELECTIONDAGINFO_H
