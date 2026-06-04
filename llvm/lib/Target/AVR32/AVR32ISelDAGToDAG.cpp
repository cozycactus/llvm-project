//===-- AVR32ISelDAGToDAG.cpp - AVR32 DAG instruction selector -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32.h"
#include "AVR32TargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "avr32-isel"
#define PASS_NAME "AVR32 DAG->DAG Pattern Instruction Selection"

#define GET_INSTRINFO_ENUM
#include "AVR32GenInstrInfo.inc"

namespace {
class AVR32DAGToDAGISel : public SelectionDAGISel {
public:
  explicit AVR32DAGToDAGISel(AVR32TargetMachine &TM, CodeGenOptLevel OptLevel)
      : SelectionDAGISel(TM, OptLevel) {}

  void Select(SDNode *Node) override {
    if (Node->isMachineOpcode()) {
      Node->setNodeId(-1);
      return;
    }
    SelectCode(Node);
  }

#include "AVR32GenDAGISel.inc"
};

class AVR32DAGToDAGISelLegacy : public SelectionDAGISelLegacy {
public:
  static char ID;
  AVR32DAGToDAGISelLegacy(AVR32TargetMachine &TM, CodeGenOptLevel OptLevel)
      : SelectionDAGISelLegacy(
            ID, std::make_unique<AVR32DAGToDAGISel>(TM, OptLevel)) {}
};
} // namespace

char AVR32DAGToDAGISelLegacy::ID;

INITIALIZE_PASS(AVR32DAGToDAGISelLegacy, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createAVR32ISelDag(AVR32TargetMachine &TM,
                                       CodeGenOptLevel OptLevel) {
  return new AVR32DAGToDAGISelLegacy(TM, OptLevel);
}
