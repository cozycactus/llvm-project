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
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "avr32-isel"
#define PASS_NAME "AVR32 DAG->DAG Pattern Instruction Selection"

#define GET_INSTRINFO_ENUM
#include "AVR32GenInstrInfo.inc"
#define GET_REGINFO_ENUM
#include "AVR32GenRegisterInfo.inc"

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

    if (selectStackLoadStore(Node))
      return;

    SelectCode(Node);
  }

  bool selectFrameIndexAddress(SDValue Addr, SDValue &Base, SDValue &Disp) {
    auto *FIN = dyn_cast<FrameIndexSDNode>(Addr);
    if (!FIN)
      return false;

    SDLoc DL(Addr);
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
    Disp = CurDAG->getTargetConstant(0, DL, MVT::i32);
    return true;
  }

  bool selectStackLoadStore(SDNode *Node) {
    SDLoc DL(Node);
    SDValue Base;
    SDValue Disp;

    if (Node->getOpcode() == ISD::LOAD) {
      auto *LD = cast<LoadSDNode>(Node);
      if (LD->getMemoryVT() != MVT::i32 ||
          !selectFrameIndexAddress(LD->getBasePtr(), Base, Disp))
        return false;

      SDValue Ops[] = {Base, Disp, LD->getChain()};
      SDNode *Result = CurDAG->SelectNodeTo(Node, AVR32::LD_W_Disp16, MVT::i32,
                                           MVT::Other, Ops);
      CurDAG->setNodeMemRefs(cast<MachineSDNode>(Result),
                             {LD->getMemOperand()});
      return true;
    }

    if (Node->getOpcode() == ISD::STORE) {
      auto *ST = cast<StoreSDNode>(Node);
      if (ST->isTruncatingStore() || ST->getMemoryVT() != MVT::i32 ||
          !selectFrameIndexAddress(ST->getBasePtr(), Base, Disp))
        return false;

      SDValue Ops[] = {Base, Disp, ST->getValue(), ST->getChain()};
      SDNode *Result =
          CurDAG->SelectNodeTo(Node, AVR32::ST_W_Disp16, MVT::Other, Ops);
      CurDAG->setNodeMemRefs(cast<MachineSDNode>(Result),
                             {ST->getMemOperand()});
      return true;
    }

    return false;
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
