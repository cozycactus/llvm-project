//===-- AVR32SelectionDAGInfo.cpp - AVR32 SelectionDAG info ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32SelectionDAGInfo.h"

#define GET_SDNODE_DESC
#include "AVR32GenSDNodeInfo.inc"

using namespace llvm;

AVR32SelectionDAGInfo::AVR32SelectionDAGInfo()
    : SelectionDAGGenTargetInfo(AVR32GenSDNodeInfo) {}
