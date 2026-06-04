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

    if (selectGlobalAddress(Node))
      return;

    if (selectConstant(Node))
      return;

    if (selectFrameIndex(Node))
      return;

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

  bool selectBaseDispAddress(SDValue Addr, SDValue &Base, SDValue &Disp) {
    if (selectFrameIndexAddress(Addr, Base, Disp) ||
        selectGlobalAddress(Addr, Base, Disp))
      return true;

    SDLoc DL(Addr);
    if (Addr.getOpcode() == ISD::ADD) {
      SDValue LHS = Addr.getOperand(0);
      SDValue RHS = Addr.getOperand(1);
      auto *C = dyn_cast<ConstantSDNode>(RHS);
      if (!C) {
        std::swap(LHS, RHS);
        C = dyn_cast<ConstantSDNode>(RHS);
      }

      if (C && isInt<16>(C->getSExtValue())) {
        Base = LHS;
        Disp = CurDAG->getTargetConstant(C->getSExtValue(), DL, MVT::i32);
        return true;
      }
    }

    Base = Addr;
    Disp = CurDAG->getTargetConstant(0, DL, MVT::i32);
    return true;
  }

  unsigned getLoadOpcode(EVT MemVT, ISD::LoadExtType ExtType) const {
    if (MemVT == MVT::i32)
      return AVR32::LD_W_Disp16;
    if (MemVT == MVT::i8 || MemVT == MVT::i1)
      return ExtType == ISD::SEXTLOAD ? AVR32::LD_SB_Disp16
                                      : AVR32::LD_UB_Disp16;
    if (MemVT == MVT::i16)
      return ExtType == ISD::SEXTLOAD ? AVR32::LD_SH_Disp16
                                      : AVR32::LD_UH_Disp16;
    return 0;
  }

  unsigned getIndexedLoadOpcode(EVT MemVT, ISD::LoadExtType ExtType) const {
    if (MemVT == MVT::i32)
      return AVR32::LD_W_IndexShift;
    if (MemVT == MVT::i8 || MemVT == MVT::i1)
      return ExtType == ISD::SEXTLOAD ? AVR32::LD_SB_IndexShift
                                      : AVR32::LD_UB_IndexShift;
    if (MemVT == MVT::i16)
      return ExtType == ISD::SEXTLOAD ? AVR32::LD_SH_IndexShift
                                      : AVR32::LD_UH_IndexShift;
    return 0;
  }

  unsigned getStoreOpcode(EVT MemVT) const {
    if (MemVT == MVT::i32)
      return AVR32::ST_W_Disp16;
    if (MemVT == MVT::i16)
      return AVR32::ST_H_Disp16;
    if (MemVT == MVT::i8 || MemVT == MVT::i1)
      return AVR32::ST_B_Disp16;
    return 0;
  }

  bool selectGlobalAddress(SDValue Addr, SDValue &Base, SDValue &Disp) {
    auto *GA = dyn_cast<GlobalAddressSDNode>(Addr);
    if (!GA)
      return false;

    SDLoc DL(Addr);
    SDValue TargetGA = CurDAG->getTargetGlobalAddress(
        GA->getGlobal(), DL, MVT::i32, GA->getOffset());
    Base = SDValue(CurDAG->getMachineNode(AVR32::MOVri21, DL, MVT::i32,
                                          TargetGA),
                   0);
    Disp = CurDAG->getTargetConstant(0, DL, MVT::i32);
    return true;
  }

  bool selectGlobalIndexedAddress(SDValue Addr, SDValue &Base, SDValue &Index,
                                  SDValue &Shift) {
    if (Addr.getOpcode() != ISD::ADD)
      return false;

    SDValue LHS = Addr.getOperand(0);
    SDValue RHS = Addr.getOperand(1);
    if (!isa<GlobalAddressSDNode>(RHS))
      std::swap(LHS, RHS);

    auto *GA = dyn_cast<GlobalAddressSDNode>(RHS);
    if (!GA)
      return false;

    uint64_t ShiftAmt = 0;
    Index = LHS;
    if (LHS.getOpcode() == ISD::SHL) {
      auto *ShiftC = dyn_cast<ConstantSDNode>(LHS.getOperand(1));
      if (!ShiftC || ShiftC->getZExtValue() > 3)
        return false;
      ShiftAmt = ShiftC->getZExtValue();
      Index = LHS.getOperand(0);
    }

    SDLoc DL(Addr);
    SDValue TargetGA = CurDAG->getTargetGlobalAddress(
        GA->getGlobal(), DL, MVT::i32, GA->getOffset());
    Base = SDValue(CurDAG->getMachineNode(AVR32::MOVri21, DL, MVT::i32,
                                          TargetGA),
                   0);
    Shift = CurDAG->getTargetConstant(ShiftAmt, DL, MVT::i32);
    return true;
  }

  bool selectGlobalAddress(SDNode *Node) {
    if (Node->getOpcode() != ISD::GlobalAddress)
      return false;

    auto *GA = cast<GlobalAddressSDNode>(Node);
    SDLoc DL(Node);
    SDValue TargetGA = CurDAG->getTargetGlobalAddress(
        GA->getGlobal(), DL, MVT::i32, GA->getOffset());
    CurDAG->SelectNodeTo(Node, AVR32::MOVri21, MVT::i32, TargetGA);
    return true;
  }

  bool selectConstant(SDNode *Node) {
    if (Node->getOpcode() != ISD::Constant || Node->getValueType(0) != MVT::i32)
      return false;

    auto *C = cast<ConstantSDNode>(Node);
    SDLoc DL(Node);
    if (isInt<21>(C->getSExtValue())) {
      SDValue Imm = CurDAG->getTargetConstant(C->getSExtValue(), DL, MVT::i32);
      CurDAG->SelectNodeTo(Node, AVR32::MOVri21, MVT::i32, Imm);
      return true;
    }

    uint32_t Bits = static_cast<uint32_t>(C->getZExtValue());
    uint32_t Hi = Bits >> 16;
    uint32_t Lo = Bits & 0xffff;
    SDValue HiImm = CurDAG->getTargetConstant(Hi, DL, MVT::i32);

    if (Lo == 0) {
      CurDAG->SelectNodeTo(Node, AVR32::MOVHri, MVT::i32, HiImm);
      return true;
    }

    SDValue HiReg =
        SDValue(CurDAG->getMachineNode(AVR32::MOVHri, DL, MVT::i32, HiImm), 0);
    SDValue LoImm = CurDAG->getTargetConstant(Lo, DL, MVT::i32);
    SDValue LoReg =
        SDValue(CurDAG->getMachineNode(AVR32::MOVri21, DL, MVT::i32, LoImm), 0);
    CurDAG->SelectNodeTo(Node, AVR32::ORALrrr, MVT::i32, HiReg, LoReg);
    return true;
  }

  bool selectFrameIndex(SDNode *Node) {
    if (Node->getOpcode() != ISD::FrameIndex)
      return false;

    auto *FI = cast<FrameIndexSDNode>(Node);
    SDLoc DL(Node);
    SDValue Frame = CurDAG->getTargetFrameIndex(FI->getIndex(), MVT::i32);
    SDValue Disp = CurDAG->getTargetConstant(0, DL, MVT::i32);
    CurDAG->SelectNodeTo(Node, AVR32::FIADDR, MVT::i32, Frame, Disp);
    return true;
  }

  bool selectStackLoadStore(SDNode *Node) {
    SDLoc DL(Node);
    SDValue Base;
    SDValue Disp;

    if (Node->getOpcode() == ISD::LOAD) {
      auto *LD = cast<LoadSDNode>(Node);
      unsigned Opcode = getLoadOpcode(LD->getMemoryVT(), LD->getExtensionType());
      unsigned IndexedOpcode =
          getIndexedLoadOpcode(LD->getMemoryVT(), LD->getExtensionType());
      SDValue Index;
      SDValue Shift;
      if (IndexedOpcode &&
          selectGlobalIndexedAddress(LD->getBasePtr(), Base, Index, Shift)) {
        SDValue Ops[] = {Base, Index, Shift, LD->getChain()};
        SDNode *Result =
            CurDAG->SelectNodeTo(Node, IndexedOpcode, MVT::i32, MVT::Other, Ops);
        CurDAG->setNodeMemRefs(cast<MachineSDNode>(Result),
                               {LD->getMemOperand()});
        return true;
      }

      if (!Opcode || !selectBaseDispAddress(LD->getBasePtr(), Base, Disp))
        return false;

      SDValue Ops[] = {Base, Disp, LD->getChain()};
      SDNode *Result =
          CurDAG->SelectNodeTo(Node, Opcode, MVT::i32, MVT::Other, Ops);
      CurDAG->setNodeMemRefs(cast<MachineSDNode>(Result),
                             {LD->getMemOperand()});
      return true;
    }

    if (Node->getOpcode() == ISD::STORE) {
      auto *ST = cast<StoreSDNode>(Node);
      unsigned Opcode = getStoreOpcode(ST->getMemoryVT());
      if (!Opcode || !selectBaseDispAddress(ST->getBasePtr(), Base, Disp))
        return false;

      SDValue Ops[] = {Base, Disp, ST->getValue(), ST->getChain()};
      SDNode *Result = CurDAG->SelectNodeTo(Node, Opcode, MVT::Other, Ops);
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
