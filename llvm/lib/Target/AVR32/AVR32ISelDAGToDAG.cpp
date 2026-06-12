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
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include <limits>

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

    if (selectJumpTableBranch(Node))
      return;

    if (selectGlobalAddress(Node))
      return;

    if (selectJumpTableAddress(Node))
      return;

    if (selectBlockAddress(Node))
      return;

    if (selectConstant(Node))
      return;

    if (selectUnsignedCastMask(Node))
      return;

    if (selectBitFieldInsert(Node))
      return;

    if (selectFrameIndex(Node))
      return;

    if (selectHalfwordImmediateOp(Node))
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
        Disp = CurDAG->getSignedTargetConstant(C->getSExtValue(), DL,
                                               MVT::i32);
        return true;
      }
    }

    Base = Addr;
    Disp = CurDAG->getTargetConstant(0, DL, MVT::i32);
    return true;
  }

  bool SelectInlineAsmMemoryOperand(
      const SDValue &Op, InlineAsm::ConstraintCode ConstraintID,
      std::vector<SDValue> &OutOps) override {
    switch (ConstraintID) {
    default:
      return true;
    case InlineAsm::ConstraintCode::m:
    case InlineAsm::ConstraintCode::o:
    case InlineAsm::ConstraintCode::Q:
      break;
    }

    SDValue Base;
    SDValue Disp;
    if (!selectBaseDispAddress(Op, Base, Disp))
      return true;

    OutOps.push_back(Base);
    OutOps.push_back(Disp);
    return false;
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

  unsigned getPostIncLoadOpcode(EVT MemVT, ISD::LoadExtType ExtType) const {
    if (MemVT == MVT::i32)
      return ExtType == ISD::NON_EXTLOAD ? AVR32::LD_W_PostInc : 0;
    if (MemVT == MVT::i8 || MemVT == MVT::i1)
      return ExtType == ISD::SEXTLOAD ? 0 : AVR32::LD_UB_PostInc;
    if (MemVT == MVT::i16)
      return ExtType == ISD::SEXTLOAD ? AVR32::LD_SH_PostInc
                                      : AVR32::LD_UH_PostInc;
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

  unsigned getPostIncStoreOpcode(EVT MemVT) const {
    if (MemVT == MVT::i32)
      return AVR32::ST_W_PostInc;
    if (MemVT == MVT::i16)
      return AVR32::ST_H_PostInc;
    if (MemVT == MVT::i8 || MemVT == MVT::i1)
      return AVR32::ST_B_PostInc;
    return 0;
  }

  static unsigned getPostIncSize(EVT VT) {
    if (VT == MVT::i8 || VT == MVT::i1)
      return 1;
    if (VT == MVT::i16)
      return 2;
    if (VT == MVT::i32)
      return 4;
    return 0;
  }

  static bool getAndConstant(SDValue Value, SDValue &Other,
                             uint32_t &Imm) {
    if (Value.getOpcode() != ISD::AND)
      return false;

    SDValue LHS = Value.getOperand(0);
    SDValue RHS = Value.getOperand(1);
    if (!getI32Constant(RHS, Imm)) {
      std::swap(LHS, RHS);
      if (!getI32Constant(RHS, Imm))
        return false;
    }

    Other = LHS;
    return true;
  }

  static bool getI32Constant(SDValue Value, uint32_t &Imm) {
    if (auto *C = dyn_cast<ConstantSDNode>(Value)) {
      Imm = static_cast<uint32_t>(C->getZExtValue());
      return true;
    }

    uint32_t LHS;
    uint32_t RHS;
    switch (Value.getOpcode()) {
    case ISD::ADD:
      if (!getI32Constant(Value.getOperand(0), LHS) ||
          !getI32Constant(Value.getOperand(1), RHS))
        return false;
      Imm = LHS + RHS;
      return true;
    case ISD::AND:
      if (!getI32Constant(Value.getOperand(0), LHS) ||
          !getI32Constant(Value.getOperand(1), RHS))
        return false;
      Imm = LHS & RHS;
      return true;
    case ISD::OR:
      if (!getI32Constant(Value.getOperand(0), LHS) ||
          !getI32Constant(Value.getOperand(1), RHS))
        return false;
      Imm = LHS | RHS;
      return true;
    case ISD::XOR:
      if (!getI32Constant(Value.getOperand(0), LHS) ||
          !getI32Constant(Value.getOperand(1), RHS))
        return false;
      Imm = LHS ^ RHS;
      return true;
    default:
      return false;
    }
  }

  static bool getShiftLeftConstant(SDValue Value, SDValue &Other,
                                   unsigned &Shift) {
    if (Value.getOpcode() != ISD::SHL)
      return false;

    auto *C = dyn_cast<ConstantSDNode>(Value.getOperand(1));
    if (!C || C->getZExtValue() >= 32)
      return false;

    Other = Value.getOperand(0);
    Shift = C->getZExtValue();
    return true;
  }

  static bool getFieldFromClearMask(uint32_t ClearMask, unsigned &BitPos,
                                    unsigned &Width) {
    uint32_t FieldMask = ~ClearMask;
    if (FieldMask == 0 || FieldMask == UINT32_MAX ||
        !isShiftedMask_32(FieldMask))
      return false;

    BitPos = llvm::countr_zero(FieldMask);
    Width = llvm::popcount(FieldMask);
    return Width > 0 && Width < 32 && BitPos + Width <= 32;
  }

  bool selectInsertSource(SDValue Value, unsigned BitPos, unsigned Width,
                          const SDLoc &DL, SDValue &Src) {
    uint32_t FieldMask = maskTrailingOnes<uint32_t>(Width) << BitPos;
    uint32_t WidthMask = maskTrailingOnes<uint32_t>(Width);
    SDValue Other;
    uint32_t Mask;

    uint32_t Imm;
    if (getI32Constant(Value, Imm)) {
      if ((Imm & ~FieldMask) == 0) {
        Src = materializeI32Constant(Imm >> BitPos, DL);
        return true;
      }
    }

    if (getAndConstant(Value, Other, Mask) && Mask == FieldMask) {
      if (BitPos == 0) {
        Src = Other;
        return true;
      }

      SDValue Shifted;
      unsigned Shift;
      if (getShiftLeftConstant(Other, Shifted, Shift) && Shift == BitPos) {
        Src = Shifted;
        return true;
      }
    }

    SDValue Shifted;
    unsigned Shift;
    if (BitPos != 0 && getShiftLeftConstant(Value, Shifted, Shift) &&
        Shift == BitPos && getAndConstant(Shifted, Other, Mask) &&
        Mask == WidthMask) {
      Src = Other;
      return true;
    }

    return false;
  }

  bool trySelectBitFieldInsert(SDNode *Node, SDValue ClearedPart,
                               SDValue InsertPart) {
    SDValue Old;
    uint32_t ClearMask;
    if (!getAndConstant(ClearedPart, Old, ClearMask))
      return false;

    unsigned BitPos;
    unsigned Width;
    if (!getFieldFromClearMask(ClearMask, BitPos, Width))
      return false;

    SDValue Src;
    SDLoc DL(Node);
    if (!selectInsertSource(InsertPart, BitPos, Width, DL, Src))
      return false;

    SDValue Bp = CurDAG->getTargetConstant(BitPos, DL, MVT::i32);
    SDValue W = CurDAG->getTargetConstant(Width, DL, MVT::i32);
    SDValue Ops[] = {Old, Src, Bp, W};
    CurDAG->SelectNodeTo(Node, AVR32::BFINScg, MVT::i32, Ops);
    return true;
  }

  bool selectBitFieldInsert(SDNode *Node) {
    if (Node->getOpcode() != ISD::OR || Node->getValueType(0) != MVT::i32)
      return false;

    return trySelectBitFieldInsert(Node, Node->getOperand(0),
                                   Node->getOperand(1)) ||
           trySelectBitFieldInsert(Node, Node->getOperand(1),
                                   Node->getOperand(0));
  }

  bool selectGlobalAddress(SDValue Addr, SDValue &Base, SDValue &Disp) {
    auto *GA = dyn_cast<GlobalAddressSDNode>(Addr);
    if (!GA)
      return false;

    SDLoc DL(Addr);
    int64_t Offset = GA->getOffset();
    if (isInt<16>(Offset)) {
      Base = materializeGlobalAddress(GA, DL, /*Offset=*/0);
      Disp = CurDAG->getSignedTargetConstant(Offset, DL, MVT::i32);
      return true;
    }

    Base = materializeGlobalAddress(GA, DL, Offset);
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
    if (isa<ConstantSDNode>(Index))
      return false;

    if (LHS.getOpcode() == ISD::SHL) {
      auto *ShiftC = dyn_cast<ConstantSDNode>(LHS.getOperand(1));
      if (!ShiftC || ShiftC->getZExtValue() > 3)
        return false;
      ShiftAmt = ShiftC->getZExtValue();
      Index = LHS.getOperand(0);
      if (isa<ConstantSDNode>(Index))
        return false;
    }

    SDLoc DL(Addr);
    Base = materializeGlobalAddress(GA, DL);
    Shift = CurDAG->getTargetConstant(ShiftAmt, DL, MVT::i32);
    return true;
  }

  SDValue materializeI32Constant(uint32_t Bits, const SDLoc &DL) {
    int32_t Signed = static_cast<int32_t>(Bits);
    if (isInt<21>(Signed)) {
      SDValue Imm = CurDAG->getSignedTargetConstant(Signed, DL, MVT::i32);
      return SDValue(CurDAG->getMachineNode(AVR32::MOVri21, DL, MVT::i32, Imm),
                     0);
    }

    const Function &F = CurDAG->getMachineFunction().getFunction();
    if (F.hasOptSize() || F.hasMinSize()) {
      SDValue Imm = CurDAG->getTargetConstant(Bits, DL, MVT::i32);
      return SDValue(CurDAG->getMachineNode(AVR32::LDA_W, DL, MVT::i32, Imm),
                     0);
    }

    uint32_t Hi = Bits >> 16;
    uint32_t Lo = Bits & 0xffff;
    SDValue LoImm = CurDAG->getTargetConstant(Lo, DL, MVT::i32);
    SDValue LoReg =
        SDValue(CurDAG->getMachineNode(AVR32::MOVri21, DL, MVT::i32, LoImm), 0);

    if (Hi == 0)
      return LoReg;

    SDValue HiImm = CurDAG->getTargetConstant(Hi, DL, MVT::i32);
    return SDValue(CurDAG->getMachineNode(AVR32::ORHcg, DL, MVT::i32, LoReg,
                                          HiImm),
                   0);
  }

  bool selectGlobalAddress(SDNode *Node) {
    if (Node->getOpcode() != ISD::GlobalAddress)
      return false;

    auto *GA = cast<GlobalAddressSDNode>(Node);
    SDLoc DL(Node);
    SDValue Result = materializeGlobalAddress(GA, DL);
    ReplaceNode(Node, Result.getNode());
    return true;
  }

  SDValue materializeGlobalAddress(
      const GlobalAddressSDNode *GA, const SDLoc &DL,
      int64_t Offset = std::numeric_limits<int64_t>::min()) {
    if (Offset == std::numeric_limits<int64_t>::min())
      Offset = GA->getOffset();

    bool SplitExternalOffset = Offset != 0 && GA->getGlobal()->isDeclaration();
    SDValue Sym = CurDAG->getTargetGlobalAddress(
        GA->getGlobal(), DL, MVT::i32, SplitExternalOffset ? 0 : Offset);
    SDValue Addr =
        SDValue(CurDAG->getMachineNode(AVR32::LDA_W, DL, MVT::i32, Sym), 0);

    if (!SplitExternalOffset)
      return Addr;

    SDValue OffsetReg = materializeI32Constant(static_cast<uint32_t>(Offset),
                                               DL);
    return SDValue(CurDAG->getMachineNode(AVR32::ADDALrrr, DL, MVT::i32, Addr,
                                          OffsetReg),
                   0);
  }

  SDValue materializeJumpTableAddress(const JumpTableSDNode *JT,
                                      const SDLoc &DL) {
    SDValue Sym =
        CurDAG->getTargetJumpTable(JT->getIndex(), MVT::i32,
                                   JT->getTargetFlags());
    return SDValue(CurDAG->getMachineNode(AVR32::LDA_W, DL, MVT::i32, Sym), 0);
  }

  SDValue materializeBlockAddress(const BlockAddressSDNode *BA,
                                  const SDLoc &DL) {
    SDValue Sym = CurDAG->getTargetBlockAddress(
        BA->getBlockAddress(), MVT::i32, BA->getOffset(),
        BA->getTargetFlags());
    return SDValue(CurDAG->getMachineNode(AVR32::LDA_W, DL, MVT::i32, Sym), 0);
  }

  bool selectJumpTableAddress(SDNode *Node) {
    if (Node->getOpcode() != ISD::JumpTable &&
        Node->getOpcode() != ISD::TargetJumpTable)
      return false;

    SDLoc DL(Node);
    SDValue Result = materializeJumpTableAddress(cast<JumpTableSDNode>(Node),
                                                 DL);
    ReplaceNode(Node, Result.getNode());
    return true;
  }

  bool selectBlockAddress(SDNode *Node) {
    if (Node->getOpcode() != ISD::BlockAddress &&
        Node->getOpcode() != ISD::TargetBlockAddress)
      return false;

    SDLoc DL(Node);
    SDValue Result = materializeBlockAddress(cast<BlockAddressSDNode>(Node),
                                             DL);
    ReplaceNode(Node, Result.getNode());
    return true;
  }

  bool selectJumpTableBranch(SDNode *Node) {
    if (Node->getOpcode() != ISD::BR_JT)
      return false;

    SDLoc DL(Node);
    SDValue Chain = Node->getOperand(0);
    SDValue Table = Node->getOperand(1);
    SDValue Index = Node->getOperand(2);
    SDValue Base = isa<JumpTableSDNode>(Table)
                       ? materializeJumpTableAddress(
                             cast<JumpTableSDNode>(Table), DL)
                       : Table;
    SDValue Shift = CurDAG->getTargetConstant(2, DL, MVT::i32);
    SDValue Ops[] = {Base, Index, Shift, Chain};
    CurDAG->SelectNodeTo(Node, AVR32::LD_W_JT, MVT::Other, Ops);
    return true;
  }

  bool selectConstant(SDNode *Node) {
    if (Node->getOpcode() != ISD::Constant || Node->getValueType(0) != MVT::i32)
      return false;

    auto *C = cast<ConstantSDNode>(Node);
    SDLoc DL(Node);
    SDValue Result =
        materializeI32Constant(static_cast<uint32_t>(C->getZExtValue()), DL);
    ReplaceNode(Node, Result.getNode());
    return true;
  }

  bool selectHalfwordImmediateOp(SDNode *Node) {
    if (Node->getValueType(0) != MVT::i32)
      return false;

    unsigned Opcode = Node->getOpcode();
    if (Opcode != ISD::AND && Opcode != ISD::OR && Opcode != ISD::XOR)
      return false;

    SDValue Src = Node->getOperand(0);
    SDValue ImmOp = Node->getOperand(1);
    auto *C = dyn_cast<ConstantSDNode>(ImmOp);
    if (!C) {
      std::swap(Src, ImmOp);
      C = dyn_cast<ConstantSDNode>(ImmOp);
    }
    if (!C)
      return false;

    uint32_t Imm = static_cast<uint32_t>(C->getZExtValue());
    uint16_t Lo = static_cast<uint16_t>(Imm);
    uint16_t Hi = static_cast<uint16_t>(Imm >> 16);
    unsigned MachineOpcode = 0;
    uint16_t Half = 0;

    switch (Opcode) {
    default:
      return false;
    case ISD::AND:
      if (has_single_bit<uint32_t>(~Imm))
        return false;
      if (Lo == 0xffff && Hi != 0xffff) {
        MachineOpcode = AVR32::ANDHcg;
        Half = Hi;
      } else if (Hi == 0xffff && Lo != 0xffff) {
        MachineOpcode = AVR32::ANDLcg;
        Half = Lo;
      }
      break;
    case ISD::OR:
      if (has_single_bit<uint32_t>(Imm))
        return false;
      if (Lo == 0 && Hi != 0) {
        MachineOpcode = AVR32::ORHcg;
        Half = Hi;
      } else if (Hi == 0 && Lo != 0) {
        MachineOpcode = AVR32::ORLcg;
        Half = Lo;
      }
      break;
    case ISD::XOR:
      if (Lo == 0 && Hi != 0) {
        MachineOpcode = AVR32::EORHcg;
        Half = Hi;
      } else if (Hi == 0 && Lo != 0) {
        MachineOpcode = AVR32::EORLcg;
        Half = Lo;
      }
      break;
    }

    if (!MachineOpcode)
      return false;

    SDLoc DL(Node);
    SDValue HalfImm = CurDAG->getTargetConstant(Half, DL, MVT::i32);
    CurDAG->SelectNodeTo(Node, MachineOpcode, MVT::i32, Src, HalfImm);
    return true;
  }

  bool selectUnsignedCastMask(SDNode *Node) {
    if (Node->getOpcode() != ISD::AND || Node->getValueType(0) != MVT::i32)
      return false;

    SDValue Src;
    auto *C = dyn_cast<ConstantSDNode>(Node->getOperand(1));
    if (C) {
      Src = Node->getOperand(0);
    } else {
      C = dyn_cast<ConstantSDNode>(Node->getOperand(0));
      if (!C)
        return false;
      Src = Node->getOperand(1);
    }

    uint64_t Mask = C->getZExtValue();
    unsigned MachineOpcode = 0;
    if (Mask == 0xff)
      MachineOpcode = AVR32::CASTU_Bcg;
    else if (Mask == 0xffff)
      MachineOpcode = AVR32::CASTU_Hcg;
    else
      return false;

    CurDAG->SelectNodeTo(Node, MachineOpcode, MVT::i32, Src);
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
      if (LD->getAddressingMode() == ISD::POST_INC) {
        unsigned Opcode =
            getPostIncLoadOpcode(LD->getMemoryVT(), LD->getExtensionType());
        auto *Offset = dyn_cast<ConstantSDNode>(LD->getOffset());
        if (!Opcode || !Offset ||
            Offset->getSExtValue() != getPostIncSize(LD->getMemoryVT()))
          return false;

        SDValue Ops[] = {LD->getBasePtr(), LD->getChain()};
        SDNode *Result = CurDAG->SelectNodeTo(
            Node, Opcode, MVT::i32, MVT::i32, MVT::Other, Ops);
        CurDAG->setNodeMemRefs(cast<MachineSDNode>(Result),
                               {LD->getMemOperand()});
        return true;
      }

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
      if (ST->getAddressingMode() == ISD::POST_INC) {
        unsigned Opcode = getPostIncStoreOpcode(ST->getMemoryVT());
        auto *Offset = dyn_cast<ConstantSDNode>(ST->getOffset());
        if (!Opcode || !Offset ||
            Offset->getSExtValue() != getPostIncSize(ST->getMemoryVT()))
          return false;

        SDValue Ops[] = {ST->getBasePtr(), ST->getValue(), ST->getChain()};
        SDNode *Result =
            CurDAG->SelectNodeTo(Node, Opcode, MVT::i32, MVT::Other, Ops);
        CurDAG->setNodeMemRefs(cast<MachineSDNode>(Result),
                               {ST->getMemOperand()});
        return true;
      }

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
