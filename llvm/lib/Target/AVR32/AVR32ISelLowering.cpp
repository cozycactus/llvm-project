//===-- AVR32ISelLowering.cpp - AVR32 DAG lowering -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32ISelLowering.h"
#include "AVR32MachineFunctionInfo.h"
#include "AVR32Subtarget.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#define GET_INSTRINFO_ENUM
#include "AVR32GenInstrInfo.inc"
#define GET_REGINFO_ENUM
#include "AVR32GenRegisterInfo.inc"

AVR32TargetLowering::AVR32TargetLowering(const TargetMachine &TM,
                                         const AVR32Subtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i32, &AVR32::GPRRegClass);
  // AVR32 uses adjacent register tuples for div/rem results. The generic
  // register-pressure DAG scheduler can trip over those untyped tuple values
  // before isel, while source scheduling keeps the tuple/extract shape stable.
  setSchedulingPreference(Sched::Source);
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::BR_CC, MVT::i64, Custom);
  setOperationAction(ISD::BR_JT, MVT::Other, Legal);
  setOperationAction(ISD::BRCOND, MVT::Other, Custom);
  setOperationAction({ISD::ADDC, ISD::ADDE, ISD::SUBC, ISD::SUBE}, MVT::i32,
                     Legal);
  setOperationAction(ISD::BSWAP, MVT::i32, Legal);
  setOperationAction(ISD::CTLZ, MVT::i32, Legal);
  setOperationAction(ISD::CTLZ_ZERO_UNDEF, MVT::i32, Legal);
  setOperationAction(ISD::CTPOP, MVT::i32, Expand);
  setOperationAction(ISD::CTTZ, MVT::i32, Legal);
  setOperationAction(ISD::CTTZ_ZERO_UNDEF, MVT::i32, Legal);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
  setOperationAction(ISD::VAARG, MVT::Other, Expand);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::MULHS, MVT::i32, Expand);
  setOperationAction(ISD::MULHU, MVT::i32, Expand);
  setOperationAction(ISD::ROTL, MVT::i32, Expand);
  setOperationAction(ISD::ROTR, MVT::i32, Expand);
  setOperationAction(ISD::FRAMEADDR, MVT::i32, Custom);
  setOperationAction(ISD::RETURNADDR, MVT::i32, Custom);
  setOperationAction(ISD::SELECT, MVT::i32, Custom);
  setOperationAction(ISD::SDIV, MVT::i32, Expand);
  setOperationAction(ISD::SDIVREM, MVT::i32, Custom);
  setOperationAction({ISD::SMAX, ISD::SMIN}, MVT::i32, Legal);
  setOperationAction(ISD::SREM, MVT::i32, Expand);
  setOperationAction(ISD::SMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::UDIV, MVT::i32, Expand);
  setOperationAction(ISD::UDIVREM, MVT::i32, Custom);
  setOperationAction(ISD::UMUL_LOHI, MVT::i32, Custom);
  setOperationAction(ISD::UREM, MVT::i32, Expand);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::i64, Custom);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);
  setOperationAction(ISD::SHL_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRA_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRL_PARTS, MVT::i32, Expand);
  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);
  setIndexedLoadAction(ISD::POST_INC, {MVT::i8, MVT::i16, MVT::i32}, Legal);
  setIndexedStoreAction(ISD::POST_INC, {MVT::i8, MVT::i16, MVT::i32}, Legal);
  // AVR32 jump tables use full-width table entries plus a materialized table
  // base, so very small switches are usually smaller as compare chains.
  setMinimumJumpTableEntries(7);
  computeRegisterProperties(STI.getRegisterInfo());
  setStackPointerRegisterToSaveRestore(AVR32::SP);
}

std::pair<unsigned, const TargetRegisterClass *>
AVR32TargetLowering::getRegForInlineAsmConstraint(
    const TargetRegisterInfo *TRI, StringRef Constraint, MVT VT) const {
  if (Constraint == "r")
    return std::make_pair(0U, &AVR32::GPRRegClass);

  return TargetLowering::getRegForInlineAsmConstraint(TRI, Constraint, VT);
}

AVR32TargetLowering::ConstraintType
AVR32TargetLowering::getConstraintType(StringRef Constraint) const {
  if (Constraint == "Ks21")
    return C_Immediate;

  return TargetLowering::getConstraintType(Constraint);
}

void AVR32TargetLowering::LowerAsmOperandForConstraint(
    SDValue Op, StringRef Constraint, std::vector<SDValue> &Ops,
    SelectionDAG &DAG) const {
  if (Constraint == "Ks21") {
    if (auto *C = dyn_cast<ConstantSDNode>(Op)) {
      int64_t Value = C->getSExtValue();
      if (isInt<21>(Value))
        Ops.push_back(DAG.getSignedTargetConstant(Value, SDLoc(Op),
                                                  Op.getValueType()));
    }
    return;
  }

  TargetLowering::LowerAsmOperandForConstraint(Op, Constraint, Ops, DAG);
}

static void diagnoseUnsupported(SelectionDAG &DAG, const SDLoc &DL,
                                StringRef Message) {
  MachineFunction &MF = DAG.getMachineFunction();
  DAG.getContext()->diagnose(
      DiagnosticInfoUnsupported(MF.getFunction(), Message, DL.getDebugLoc()));
}

static const CondCodeSDNode *
getCondCodeNode(SDValue Op, unsigned Operand, SelectionDAG &DAG,
                const SDLoc &DL) {
  const auto *CCNode = dyn_cast<CondCodeSDNode>(Op.getOperand(Operand));
  if (!CCNode)
    diagnoseUnsupported(DAG, DL, "AVR32 malformed condition code operand");
  return CCNode;
}

static bool isSupportedRegValueType(EVT VT) {
  return VT == MVT::i32 || VT == MVT::i16 || VT == MVT::i8;
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

static bool isSupportedPostIncLoad(EVT MemVT, ISD::LoadExtType ExtType) {
  if (MemVT == MVT::i32)
    return ExtType == ISD::NON_EXTLOAD;
  if (MemVT == MVT::i16)
    return true;
  if (MemVT == MVT::i8 || MemVT == MVT::i1)
    return ExtType != ISD::SEXTLOAD;
  return false;
}

static bool isSupportedPostIncStore(EVT MemVT) {
  return MemVT == MVT::i32 || MemVT == MVT::i16 || MemVT == MVT::i8 ||
         MemVT == MVT::i1;
}

static SDValue lowerDivRem(SDValue Op, SelectionDAG &DAG, unsigned DivRemOpcode,
                           unsigned DivOpcode) {
  SDLoc DL(Op);
  if (!Op.getNode()->hasAnyUseOfValue(1)) {
    SDValue Quot = DAG.getNode(DivOpcode, DL, MVT::i32, Op.getOperand(0),
                               Op.getOperand(1));
    return DAG.getMergeValues({Quot, DAG.getUNDEF(MVT::i32)}, DL);
  }

  SDValue Pair = DAG.getNode(DivRemOpcode, DL, MVT::Untyped, Op.getOperand(0),
                             Op.getOperand(1));
  SDValue Quot = DAG.getTargetExtractSubreg(sub_lo, DL, MVT::i32, Pair);
  SDValue Product = DAG.getNode(ISD::MUL, DL, MVT::i32, Quot, Op.getOperand(1));
  SDValue Rem = DAG.getNode(ISD::SUB, DL, MVT::i32, Op.getOperand(0), Product);
  return DAG.getMergeValues({Quot, Rem}, DL);
}

static SDValue lowerUMulLoHi(SDValue Op, SelectionDAG &DAG) {
  SDLoc DL(Op);
  SDValue Pair = DAG.getNode(AVR32ISD::UMUL_LOHI, DL, MVT::Untyped,
                             Op.getOperand(0), Op.getOperand(1));
  SDValue Lo = DAG.getTargetExtractSubreg(sub_lo, DL, MVT::i32, Pair);
  SDValue Hi = DAG.getTargetExtractSubreg(sub_hi, DL, MVT::i32, Pair);
  return DAG.getMergeValues({Lo, Hi}, DL);
}

static ArrayRef<MCPhysReg> getIntArgRegs() {
  static const MCPhysReg Regs[] = {AVR32::R12, AVR32::R11, AVR32::R10,
                                   AVR32::R9, AVR32::R8};
  return Regs;
}

static StringRef getCalleeName(SDValue Callee) {
  if (auto *G = dyn_cast<GlobalAddressSDNode>(Callee))
    return G->getGlobal()->getName();
  if (auto *E = dyn_cast<ExternalSymbolSDNode>(Callee))
    return E->getSymbol();
  return StringRef();
}

static bool hasAVR32F64InputABI(StringRef Name) {
  return Name == "__avr32_f64_add" || Name == "__avr32_f64_sub" ||
         Name == "__avr32_f64_mul" || Name == "__avr32_f64_div" ||
         Name == "__avr32_f64_to_s32" || Name == "__avr32_f64_to_u32" ||
         Name == "__eqdf2" || Name == "__nedf2" || Name == "__gedf2" ||
         Name == "__ltdf2" || Name == "__ledf2" || Name == "__gtdf2" ||
         Name == "__unorddf2";
}

static bool hasAVR32F64ReturnABI(StringRef Name) {
  return Name == "__avr32_f64_add" || Name == "__avr32_f64_sub" ||
         Name == "__avr32_f64_mul" || Name == "__avr32_f64_div" ||
         Name == "__avr32_s32_to_f64" || Name == "__avr32_u32_to_f64" ||
         Name == "__extendsfdf2";
}

static SDValue extendToI32(SDValue Value, const ISD::ArgFlagsTy &Flags,
                           const SDLoc &DL, SelectionDAG &DAG) {
  if (Value.getValueType() == MVT::i32)
    return Value;
  if (Flags.isSExt())
    return DAG.getNode(ISD::SIGN_EXTEND, DL, MVT::i32, Value);
  if (Flags.isZExt())
    return DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i32, Value);
  return DAG.getNode(ISD::ANY_EXTEND, DL, MVT::i32, Value);
}

static AVR32CC::CondCodes intCondCodeToAVR32CC(ISD::CondCode CC) {
  switch (CC) {
  default:
    return AVR32CC::COND_INVALID;
  case ISD::SETEQ:
    return AVR32CC::COND_EQ;
  case ISD::SETNE:
    return AVR32CC::COND_NE;
  case ISD::SETUGE:
    return AVR32CC::COND_CC;
  case ISD::SETULT:
    return AVR32CC::COND_CS;
  case ISD::SETGE:
    return AVR32CC::COND_GE;
  case ISD::SETLT:
    return AVR32CC::COND_LT;
  case ISD::SETULE:
    return AVR32CC::COND_LS;
  case ISD::SETGT:
    return AVR32CC::COND_GT;
  case ISD::SETLE:
    return AVR32CC::COND_LE;
  case ISD::SETUGT:
    return AVR32CC::COND_HI;
  }
}

static bool commuteCondCodeForCompactBranch(ISD::CondCode &CC) {
  switch (CC) {
  default:
    return false;
  case ISD::SETGT:
    CC = ISD::SETLT;
    return true;
  case ISD::SETLE:
    CC = ISD::SETGE;
    return true;
  case ISD::SETUGT:
    CC = ISD::SETULT;
    return true;
  case ISD::SETULE:
    CC = ISD::SETUGE;
    return true;
  }
}

static bool isAlwaysTrueCondCode(ISD::CondCode CC) {
  return CC == ISD::SETTRUE || CC == ISD::SETTRUE2;
}

static bool isAlwaysFalseCondCode(ISD::CondCode CC) {
  return CC == ISD::SETFALSE || CC == ISD::SETFALSE2;
}

static SDValue extractI64Word(SDValue Value, unsigned Word, const SDLoc &DL,
                              SelectionDAG &DAG) {
  return DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i32, Value,
                     DAG.getConstant(Word, DL, MVT::i32));
}

static SDValue lowerI64BR_CC(SDValue Chain, SDValue Dest, ISD::CondCode CC,
                             SDValue LHS, SDValue RHS, const SDLoc &DL,
                             SelectionDAG &DAG) {
  if (!isa<ConstantSDNode>(RHS) && commuteCondCodeForCompactBranch(CC))
    std::swap(LHS, RHS);

  AVR32CC::CondCodes TargetCC = intCondCodeToAVR32CC(CC);
  if (TargetCC == AVR32CC::COND_INVALID) {
    diagnoseUnsupported(DAG, DL, "AVR32 condition code is not implemented yet");
    return Chain;
  }

  SDValue LHSLo = extractI64Word(LHS, 0, DL, DAG);
  SDValue LHSHi = extractI64Word(LHS, 1, DL, DAG);
  SDValue RHSLo = extractI64Word(RHS, 0, DL, DAG);
  SDValue RHSHi = extractI64Word(RHS, 1, DL, DAG);
  SDValue LowFlag = DAG.getNode(AVR32ISD::CMP, DL, MVT::Glue, LHSLo, RHSLo);
  SDValue HighFlag =
      DAG.getNode(AVR32ISD::CMP_CARRY, DL, MVT::Glue, LHSHi, RHSHi, LowFlag);
  return DAG.getNode(AVR32ISD::BR_CC, DL, MVT::Other, Chain, Dest,
                     DAG.getConstant(TargetCC, DL, MVT::i32), HighFlag);
}

static SDValue lowerI64SET_CC(ISD::CondCode CC, SDValue LHS, SDValue RHS,
                              const SDLoc &DL, SelectionDAG &DAG) {
  if (commuteCondCodeForCompactBranch(CC))
    std::swap(LHS, RHS);

  AVR32CC::CondCodes TargetCC = intCondCodeToAVR32CC(CC);
  if (TargetCC == AVR32CC::COND_INVALID) {
    diagnoseUnsupported(DAG, DL, "AVR32 condition code is not implemented yet");
    return DAG.getUNDEF(MVT::i32);
  }

  SDValue LHSLo = extractI64Word(LHS, 0, DL, DAG);
  SDValue LHSHi = extractI64Word(LHS, 1, DL, DAG);
  SDValue RHSLo = extractI64Word(RHS, 0, DL, DAG);
  SDValue RHSHi = extractI64Word(RHS, 1, DL, DAG);
  return DAG.getNode(AVR32ISD::SET_CC_64, DL, MVT::i32, LHSLo, LHSHi, RHSLo,
                     RHSHi, DAG.getTargetConstant(TargetCC, DL, MVT::i32));
}

static SDValue lowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MFI.setFrameAddressIsTaken(true);

  EVT VT = Op.getValueType();
  SDLoc DL(Op);
  unsigned Depth = Op.getConstantOperandVal(0);
  if (Depth != 0) {
    diagnoseUnsupported(DAG, DL,
                        "AVR32 frame addresses beyond depth 0 are not "
                        "implemented yet");
    return DAG.getUNDEF(VT);
  }

  return DAG.getCopyFromReg(DAG.getEntryNode(), DL, AVR32::R7, VT);
}

static SDValue lowerRETURNADDR(SDValue Op, SelectionDAG &DAG) {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MFI.setReturnAddressIsTaken(true);

  EVT VT = Op.getValueType();
  SDLoc DL(Op);
  unsigned Depth = Op.getConstantOperandVal(0);
  if (Depth != 0) {
    diagnoseUnsupported(DAG, DL,
                        "AVR32 return addresses beyond depth 0 are not "
                        "implemented yet");
    return DAG.getUNDEF(VT);
  }

  Register Reg = MF.addLiveIn(AVR32::LR, &AVR32::GPRRegClass);
  return DAG.getCopyFromReg(DAG.getEntryNode(), DL, Reg, VT);
}

static SDValue lowerVASTART(SDValue Op, SelectionDAG &DAG) {
  MachineFunction &MF = DAG.getMachineFunction();
  const AVR32MachineFunctionInfo *FuncInfo =
      MF.getInfo<AVR32MachineFunctionInfo>();
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  EVT PtrVT = DAG.getTargetLoweringInfo().getPointerTy(DAG.getDataLayout());
  SDLoc DL(Op);

  SDValue FI = DAG.getFrameIndex(FuncInfo->getVarArgsFrameIndex(), PtrVT);
  return DAG.getStore(Op.getOperand(0), DL, FI, Op.getOperand(1),
                      MachinePointerInfo(SV));
}

SDValue AVR32TargetLowering::LowerOperation(SDValue Op,
                                            SelectionDAG &DAG) const {
  if (Op.getOpcode() == ISD::FRAMEADDR)
    return lowerFRAMEADDR(Op, DAG);
  if (Op.getOpcode() == ISD::RETURNADDR)
    return lowerRETURNADDR(Op, DAG);
  if (Op.getOpcode() == ISD::VASTART)
    return lowerVASTART(Op, DAG);
  if (Op.getOpcode() == ISD::SDIVREM)
    return lowerDivRem(Op, DAG, AVR32ISD::SDIVREM, AVR32ISD::SDIV);
  if (Op.getOpcode() == ISD::UDIVREM)
    return lowerDivRem(Op, DAG, AVR32ISD::UDIVREM, AVR32ISD::UDIV);
  if (Op.getOpcode() == ISD::UMUL_LOHI)
    return lowerUMulLoHi(Op, DAG);

  if (Op.getOpcode() != ISD::BR_CC && Op.getOpcode() != ISD::BRCOND &&
      Op.getOpcode() != ISD::SELECT && Op.getOpcode() != ISD::SETCC &&
      Op.getOpcode() != ISD::SELECT_CC)
    llvm_unreachable("Unsupported AVR32 custom lowering");

  if (Op.getOpcode() == ISD::SELECT) {
    SDLoc DL(Op);
    return DAG.getNode(AVR32ISD::SELECT_CC, DL, MVT::i32, Op.getOperand(0),
                       DAG.getConstant(0, DL, MVT::i32), Op.getOperand(1),
                       Op.getOperand(2),
                       DAG.getTargetConstant(AVR32CC::COND_NE, DL, MVT::i32));
  }

  if (Op.getOpcode() == ISD::SETCC || Op.getOpcode() == ISD::SELECT_CC) {
    unsigned CondOperand = Op.getOpcode() == ISD::SETCC ? 2 : 4;
    SDLoc DL(Op);
    const CondCodeSDNode *CCNode = getCondCodeNode(Op, CondOperand, DAG, DL);
    if (!CCNode)
      return DAG.getUNDEF(Op.getValueType());
    ISD::CondCode CC = CCNode->get();
    if (isAlwaysTrueCondCode(CC)) {
      if (Op.getOpcode() == ISD::SETCC)
        return DAG.getConstant(1, DL, Op.getValueType());
      return Op.getOperand(2);
    }
    if (isAlwaysFalseCondCode(CC)) {
      if (Op.getOpcode() == ISD::SETCC)
        return DAG.getConstant(0, DL, Op.getValueType());
      return Op.getOperand(3);
    }

    SDValue LHS = Op.getOperand(0);
    SDValue RHS = Op.getOperand(1);
    if (Op.getOpcode() == ISD::SETCC && LHS.getValueType() == MVT::i64 &&
        RHS.getValueType() == MVT::i64)
      return lowerI64SET_CC(CC, LHS, RHS, DL, DAG);

    if (commuteCondCodeForCompactBranch(CC))
      std::swap(LHS, RHS);
    AVR32CC::CondCodes TargetCC = intCondCodeToAVR32CC(CC);
    if (TargetCC == AVR32CC::COND_INVALID) {
      diagnoseUnsupported(DAG, DL,
                          "AVR32 condition code is not implemented yet");
      return DAG.getUNDEF(Op.getValueType());
    }

    if (Op.getOpcode() == ISD::SELECT_CC)
      return DAG.getNode(AVR32ISD::SELECT_CC, DL, MVT::i32, LHS, RHS,
                         Op.getOperand(2), Op.getOperand(3),
                         DAG.getTargetConstant(TargetCC, DL, MVT::i32));

    return DAG.getNode(AVR32ISD::SET_CC, DL, MVT::i32, LHS, RHS,
                       DAG.getTargetConstant(TargetCC, DL, MVT::i32));
  }

  if (Op.getOpcode() == ISD::BRCOND) {
    SDValue Chain = Op.getOperand(0);
    SDValue Cond = Op.getOperand(1);
    SDValue Dest = Op.getOperand(2);
    SDLoc DL(Op);

    if (Cond.getOpcode() == ISD::AND) {
      if (auto *Mask = dyn_cast<ConstantSDNode>(Cond.getOperand(1));
          Mask && Mask->getZExtValue() == 1)
        Cond = Cond.getOperand(0);
      else if (auto *Mask = dyn_cast<ConstantSDNode>(Cond.getOperand(0));
               Mask && Mask->getZExtValue() == 1)
        Cond = Cond.getOperand(1);
    }

    if (Cond.getOpcode() == AVR32ISD::SET_CC) {
      const auto *CCNode = dyn_cast<ConstantSDNode>(Cond.getOperand(2));
      if (!CCNode)
        return Chain;
      SDValue Flag = DAG.getNode(AVR32ISD::CMP, DL, MVT::Glue,
                                 Cond.getOperand(0), Cond.getOperand(1));
      return DAG.getNode(AVR32ISD::BR_CC, DL, MVT::Other, Chain, Dest,
                         DAG.getConstant(CCNode->getZExtValue(), DL, MVT::i32),
                         Flag);
    }

    if (Cond.getOpcode() == AVR32ISD::SET_CC_64) {
      const auto *CCNode = dyn_cast<ConstantSDNode>(Cond.getOperand(4));
      if (!CCNode)
        return Chain;
      SDValue LowFlag =
          DAG.getNode(AVR32ISD::CMP, DL, MVT::Glue, Cond.getOperand(0),
                      Cond.getOperand(2));
      SDValue HighFlag =
          DAG.getNode(AVR32ISD::CMP_CARRY, DL, MVT::Glue, Cond.getOperand(1),
                      Cond.getOperand(3), LowFlag);
      return DAG.getNode(AVR32ISD::BR_CC, DL, MVT::Other, Chain, Dest,
                         DAG.getConstant(CCNode->getZExtValue(), DL, MVT::i32),
                         HighFlag);
    }

    if (Cond.getOpcode() == ISD::SETCC) {
      const CondCodeSDNode *CCNode = getCondCodeNode(Cond, 2, DAG, DL);
      if (!CCNode)
        return Chain;
      ISD::CondCode CC = CCNode->get();
      SDValue LHS = Cond.getOperand(0);
      SDValue RHS = Cond.getOperand(1);
      if (isAlwaysTrueCondCode(CC))
        return DAG.getNode(ISD::BR, DL, MVT::Other, Chain, Dest);
      if (isAlwaysFalseCondCode(CC))
        return Chain;
      if (LHS.getValueType() == MVT::i64 && RHS.getValueType() == MVT::i64)
        return lowerI64BR_CC(Chain, Dest, CC, LHS, RHS, DL, DAG);

      // Keep register-immediate branches in their original order so CPri can
      // match.
      if (!isa<ConstantSDNode>(RHS) && commuteCondCodeForCompactBranch(CC))
        std::swap(LHS, RHS);

      AVR32CC::CondCodes TargetCC = intCondCodeToAVR32CC(CC);
      if (TargetCC == AVR32CC::COND_INVALID) {
        diagnoseUnsupported(DAG, DL,
                            "AVR32 condition code is not implemented yet");
        return Chain;
      }

      SDValue Flag = DAG.getNode(AVR32ISD::CMP, DL, MVT::Glue, LHS, RHS);
      return DAG.getNode(AVR32ISD::BR_CC, DL, MVT::Other, Chain, Dest,
                         DAG.getConstant(TargetCC, DL, MVT::i32), Flag);
    }

    if (Cond.getValueType() != MVT::i32)
      Cond = DAG.getNode(ISD::ZERO_EXTEND, DL, MVT::i32, Cond);
    SDValue Flag = DAG.getNode(AVR32ISD::CMP, DL, MVT::Glue, Cond,
                               DAG.getConstant(0, DL, MVT::i32));
    return DAG.getNode(AVR32ISD::BR_CC, DL, MVT::Other, Chain, Dest,
                       DAG.getConstant(AVR32CC::COND_NE, DL, MVT::i32), Flag);
  }

  SDValue Chain = Op.getOperand(0);
  SDLoc DL(Op);
  const CondCodeSDNode *CCNode = getCondCodeNode(Op, 1, DAG, DL);
  if (!CCNode)
    return Chain;
  ISD::CondCode CC = CCNode->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  if (isAlwaysTrueCondCode(CC))
    return DAG.getNode(ISD::BR, DL, MVT::Other, Chain, Dest);
  if (isAlwaysFalseCondCode(CC))
    return Chain;
  if (LHS.getValueType() == MVT::i64 && RHS.getValueType() == MVT::i64)
    return lowerI64BR_CC(Chain, Dest, CC, LHS, RHS, DL, DAG);

  // Keep register-immediate branches in their original order so CPri can match.
  if (!isa<ConstantSDNode>(RHS) && commuteCondCodeForCompactBranch(CC))
    std::swap(LHS, RHS);

  AVR32CC::CondCodes TargetCC = intCondCodeToAVR32CC(CC);
  if (TargetCC == AVR32CC::COND_INVALID) {
    diagnoseUnsupported(DAG, DL,
                        "AVR32 condition code is not implemented yet");
    return Chain;
  }

  SDValue Flag = DAG.getNode(AVR32ISD::CMP, DL, MVT::Glue, LHS, RHS);
  return DAG.getNode(AVR32ISD::BR_CC, DL, MVT::Other, Chain, Dest,
                     DAG.getConstant(TargetCC, DL, MVT::i32), Flag);
}

static unsigned getSetRegOpcodeForCC(AVR32CC::CondCodes CC) {
  switch (CC) {
  default:
    return 0;
  case AVR32CC::COND_EQ:
    return AVR32::SREQr;
  case AVR32CC::COND_NE:
    return AVR32::SRNEr;
  case AVR32CC::COND_CC:
    return AVR32::SRCCr;
  case AVR32CC::COND_CS:
    return AVR32::SRCSr;
  case AVR32CC::COND_GE:
    return AVR32::SRGEr;
  case AVR32CC::COND_LT:
    return AVR32::SRLTr;
  case AVR32CC::COND_LS:
    return AVR32::SRLSr;
  case AVR32CC::COND_GT:
    return AVR32::SRGTr;
  case AVR32CC::COND_LE:
    return AVR32::SRLEr;
  case AVR32CC::COND_HI:
    return AVR32::SRHIr;
  }
}

static unsigned getBranchOpcodeForCC(AVR32CC::CondCodes CC) {
  switch (CC) {
  default:
    return 0;
  case AVR32CC::COND_EQ:
    return AVR32::BREQbb;
  case AVR32CC::COND_NE:
    return AVR32::BRNEbb;
  case AVR32CC::COND_CC:
    return AVR32::BRCCbb;
  case AVR32CC::COND_CS:
    return AVR32::BRCSbb;
  case AVR32CC::COND_GE:
    return AVR32::BRGEbb;
  case AVR32CC::COND_LT:
    return AVR32::BRLTbb;
  case AVR32CC::COND_LS:
    return AVR32::BRLSbb;
  case AVR32CC::COND_GT:
    return AVR32::BRGTbb;
  case AVR32CC::COND_LE:
    return AVR32::BRLEbb;
  case AVR32CC::COND_HI:
    return AVR32::BRHIbb;
  }
}

static unsigned getMoveRegOpcodeForCC(AVR32CC::CondCodes CC) {
  switch (CC) {
  default:
    return 0;
  case AVR32CC::COND_EQ:
    return AVR32::MOVEQrrCG;
  case AVR32CC::COND_NE:
    return AVR32::MOVNErrCG;
  case AVR32CC::COND_CC:
    return AVR32::MOVCCrrCG;
  case AVR32CC::COND_CS:
    return AVR32::MOVCSrrCG;
  case AVR32CC::COND_GE:
    return AVR32::MOVGErrCG;
  case AVR32CC::COND_LT:
    return AVR32::MOVLTrrCG;
  case AVR32CC::COND_LS:
    return AVR32::MOVLSrrCG;
  case AVR32CC::COND_GT:
    return AVR32::MOVGTrrCG;
  case AVR32CC::COND_LE:
    return AVR32::MOVLErrCG;
  case AVR32CC::COND_HI:
    return AVR32::MOVHIrrCG;
  }
}

MachineBasicBlock *AVR32TargetLowering::EmitInstrWithCustomInserter(
    MachineInstr &MI, MachineBasicBlock *BB) const {
  assert((MI.getOpcode() == AVR32::SETCCrr ||
          MI.getOpcode() == AVR32::SETCC64rr ||
          MI.getOpcode() == AVR32::SELECTCCrr) &&
         "Unexpected custom inserter");

  MachineFunction *MF = BB->getParent();
  const TargetInstrInfo &TII = *MF->getSubtarget().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();

  Register Dst = MI.getOperand(0).getReg();
  unsigned CondOperand = MI.getOpcode() == AVR32::SETCCrr ? 3 : 5;
  auto CC = static_cast<AVR32CC::CondCodes>(MI.getOperand(CondOperand).getImm());

  if (MI.getOpcode() == AVR32::SETCCrr ||
      MI.getOpcode() == AVR32::SETCC64rr) {
    unsigned SrOpc = getSetRegOpcodeForCC(CC);
    if (!SrOpc)
      report_fatal_error("AVR32 condition code is not implemented yet");

    if (MI.getOpcode() == AVR32::SETCCrr) {
      BuildMI(*BB, MI, DL, TII.get(AVR32::CPrr))
          .addReg(MI.getOperand(1).getReg())
          .addReg(MI.getOperand(2).getReg());
    } else {
      BuildMI(*BB, MI, DL, TII.get(AVR32::CPrr))
          .addReg(MI.getOperand(1).getReg())
          .addReg(MI.getOperand(3).getReg());
      BuildMI(*BB, MI, DL, TII.get(AVR32::CPCrr))
          .addReg(MI.getOperand(2).getReg())
          .addReg(MI.getOperand(4).getReg());
    }
    BuildMI(*BB, MI, DL, TII.get(SrOpc), Dst);
    MI.eraseFromParent();
    return BB;
  }

  Register LHS = MI.getOperand(1).getReg();
  Register RHS = MI.getOperand(2).getReg();

  // AVR32 compact branches are often smaller than full conditional moves after
  // relaxation, especially when register allocation needs an extra copy. Keep
  // that shape for -Oz/minsize, but use conditional moves for -Os where Linux
  // has many select-like patterns and avoids the extra block/PHI structure.
  if (!MF->getFunction().hasMinSize()) {
    unsigned MovOpc = getMoveRegOpcodeForCC(CC);
    if (!MovOpc)
      report_fatal_error("AVR32 condition code is not implemented yet");

    Register TrueReg = MI.getOperand(3).getReg();
    Register FalseReg = MI.getOperand(4).getReg();
    BuildMI(*BB, MI, DL, TII.get(AVR32::CPrr)).addReg(LHS).addReg(RHS);
    BuildMI(*BB, MI, DL, TII.get(MovOpc), Dst)
        .addReg(FalseReg)
        .addReg(TrueReg);
    MI.eraseFromParent();
    return BB;
  }

  unsigned BranchOpc = getBranchOpcodeForCC(CC);
  if (!BranchOpc)
    report_fatal_error("AVR32 condition code is not implemented yet");

  const BasicBlock *LLVMBB = BB->getBasicBlock();
  MachineFunction::iterator InsertPt = std::next(BB->getIterator());
  MachineBasicBlock *TrueBB = MF->CreateMachineBasicBlock(LLVMBB);
  MachineBasicBlock *FalseBB = MF->CreateMachineBasicBlock(LLVMBB);
  MachineBasicBlock *SinkBB = MF->CreateMachineBasicBlock(LLVMBB);
  MF->insert(InsertPt, TrueBB);
  MF->insert(InsertPt, FalseBB);
  MF->insert(InsertPt, SinkBB);

  SinkBB->splice(SinkBB->begin(), BB, std::next(MI.getIterator()), BB->end());
  SinkBB->transferSuccessorsAndUpdatePHIs(BB);

  BB->addSuccessor(TrueBB);
  BB->addSuccessor(FalseBB);
  BuildMI(BB, DL, TII.get(AVR32::CPrr)).addReg(LHS).addReg(RHS);
  BuildMI(BB, DL, TII.get(BranchOpc)).addMBB(TrueBB);
  BuildMI(BB, DL, TII.get(AVR32::BRALbb)).addMBB(FalseBB);

  Register TrueReg = MI.getOperand(3).getReg();
  Register FalseReg = MI.getOperand(4).getReg();

  BuildMI(TrueBB, DL, TII.get(AVR32::BRALbb)).addMBB(SinkBB);
  TrueBB->addSuccessor(SinkBB);

  BuildMI(FalseBB, DL, TII.get(AVR32::BRALbb)).addMBB(SinkBB);
  FalseBB->addSuccessor(SinkBB);

  BuildMI(*SinkBB, SinkBB->begin(), DL, TII.get(TargetOpcode::PHI), Dst)
      .addReg(TrueReg)
      .addMBB(TrueBB)
      .addReg(FalseReg)
      .addMBB(FalseBB);

  MI.eraseFromParent();
  return SinkBB;
}

bool AVR32TargetLowering::getPostIndexedAddressParts(
    SDNode *N, SDNode *Op, SDValue &Base, SDValue &Offset,
    ISD::MemIndexedMode &AM, SelectionDAG &DAG) const {
  EVT MemVT;
  SDValue Ptr;

  if (const auto *LD = dyn_cast<LoadSDNode>(N)) {
    MemVT = LD->getMemoryVT();
    Ptr = LD->getBasePtr();
    if (!isSupportedPostIncLoad(MemVT, LD->getExtensionType()))
      return false;
  } else if (const auto *ST = dyn_cast<StoreSDNode>(N)) {
    MemVT = ST->getMemoryVT();
    Ptr = ST->getBasePtr();
    if (!isSupportedPostIncStore(MemVT))
      return false;
  } else {
    return false;
  }

  if (Op->getOpcode() != ISD::ADD)
    return false;

  auto *RHS = dyn_cast<ConstantSDNode>(Op->getOperand(1));
  unsigned Inc = getPostIncSize(MemVT);
  if (!RHS || RHS->getSExtValue() != Inc)
    return false;

  Base = Op->getOperand(0);
  if (Ptr != Base)
    return false;

  Offset = DAG.getConstant(Inc, SDLoc(N), MVT::i32);
  AM = ISD::POST_INC;
  return true;
}

SDValue AVR32TargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  ArrayRef<MCPhysReg> ArgRegs = getIntArgRegs();
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  for (unsigned I = 0, E = Ins.size(); I != E; ++I) {
    if (!isSupportedRegValueType(Ins[I].VT)) {
      diagnoseUnsupported(DAG, DL,
                          "AVR32 non-integer register arguments are not "
                          "implemented yet");
      InVals.push_back(DAG.getUNDEF(Ins[I].VT));
      continue;
    }

    SDValue Arg;
    if (I < std::size(ArgRegs)) {
      Register VReg = MRI.createVirtualRegister(&AVR32::GPRRegClass);
      MRI.addLiveIn(ArgRegs[I], VReg);
      Arg = DAG.getCopyFromReg(Chain, DL, VReg, MVT::i32);
    } else {
      unsigned Offset = (I - std::size(ArgRegs)) * 4;
      int FI = MFI.CreateFixedObject(4, Offset, /*IsImmutable=*/true);
      SDValue Ptr = DAG.getFrameIndex(FI, MVT::i32);
      Arg = DAG.getLoad(MVT::i32, DL, Chain, Ptr,
                        MachinePointerInfo::getFixedStack(MF, FI));
    }

    if (Ins[I].VT != MVT::i32)
      Arg = DAG.getNode(ISD::TRUNCATE, DL, Ins[I].VT, Arg);
    InVals.push_back(Arg);
  }

  if (IsVarArg) {
    AVR32MachineFunctionInfo *FuncInfo =
        MF.getInfo<AVR32MachineFunctionInfo>();
    // AVR32 GCC passes unnamed variadic arguments on the stack, even when
    // integer argument registers are still available.
    unsigned Offset = Ins.size() > ArgRegs.size()
                          ? (Ins.size() - ArgRegs.size()) * 4
                          : 0;
    FuncInfo->setVarArgsFrameIndex(
        MFI.CreateFixedObject(4, Offset, /*IsImmutable=*/true));
  }
  return Chain;
}

SDValue
AVR32TargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                               SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc DL = CLI.DL;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  StringRef CalleeName = getCalleeName(Callee);
  CLI.IsTailCall = false;

  SmallVector<MCPhysReg, 5> ArgRegs(getIntArgRegs());
  if (hasAVR32F64InputABI(CalleeName)) {
    ArgRegs.clear();
    if (CLI.Outs.size() >= 2) {
      ArgRegs.push_back(AVR32::R11);
      ArgRegs.push_back(AVR32::R10);
    }
    if (CLI.Outs.size() >= 4) {
      ArgRegs.push_back(AVR32::R9);
      ArgRegs.push_back(AVR32::R8);
    }
  }

  unsigned RegArgLimit = ArgRegs.size();
  if (CLI.IsVarArg)
    RegArgLimit = std::min<unsigned>(RegArgLimit, CLI.NumFixedArgs);

  unsigned StackBytes =
      CLI.Outs.size() > RegArgLimit
          ? alignTo((CLI.Outs.size() - RegArgLimit) * 4, 4)
          : 0;
  Chain = DAG.getCALLSEQ_START(Chain, StackBytes, 0, DL);

  SDValue Glue;
  SmallVector<std::pair<MCPhysReg, SDValue>, 5> RegsToPass;
  SmallVector<SDValue, 8> MemOpChains;
  SDValue StackPtr;
  for (unsigned I = 0, E = CLI.Outs.size(); I != E; ++I) {
    if (!isSupportedRegValueType(CLI.Outs[I].VT)) {
      diagnoseUnsupported(DAG, DL,
                          "AVR32 non-integer register call arguments are not "
                          "implemented yet");
      continue;
    }

    SDValue Arg = extendToI32(CLI.OutVals[I], CLI.Outs[I].Flags, DL, DAG);
    if (I < RegArgLimit) {
      RegsToPass.push_back({ArgRegs[I], Arg});
      continue;
    }

    if (!StackPtr)
      StackPtr = DAG.getCopyFromReg(Chain, DL, AVR32::SP, MVT::i32);

    unsigned Offset = (I - RegArgLimit) * 4;
    SDValue Ptr =
        DAG.getNode(ISD::ADD, DL, MVT::i32, StackPtr,
                    DAG.getIntPtrConstant(Offset, DL, MVT::i32));
    MemOpChains.push_back(
        DAG.getStore(Chain, DL, Arg, Ptr, MachinePointerInfo()));
  }

  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemOpChains);

  for (const auto &[Reg, Value] : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, DL, Reg, Value, Glue);
    Glue = Chain.getValue(1);
  }

  if (auto *G = dyn_cast<GlobalAddressSDNode>(Callee))
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), DL, MVT::i32);
  else if (auto *E = dyn_cast<ExternalSymbolSDNode>(Callee))
    Callee = DAG.getTargetExternalSymbol(E->getSymbol(), MVT::i32);

  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);
  for (const auto &[Reg, Value] : RegsToPass)
    Ops.push_back(DAG.getRegister(Reg, Value.getValueType()));
  const TargetRegisterInfo *TRI = DAG.getSubtarget().getRegisterInfo();
  Ops.push_back(DAG.getRegisterMask(
      TRI->getCallPreservedMask(DAG.getMachineFunction(), CLI.CallConv)));
  if (Glue)
    Ops.push_back(Glue);

  Chain = DAG.getNode(AVR32ISD::CALL, DL,
                      DAG.getVTList(MVT::Other, MVT::Glue), Ops);
  Glue = Chain.getValue(1);

  Chain = DAG.getCALLSEQ_END(Chain, StackBytes, 0, Glue, DL);
  Glue = Chain.getValue(1);

  SmallVector<MCPhysReg, 5> RetRegs(getIntArgRegs());
  if (hasAVR32F64ReturnABI(CalleeName) && CLI.Ins.size() >= 2) {
    RetRegs.clear();
    RetRegs.push_back(AVR32::R11);
    RetRegs.push_back(AVR32::R10);
  }

  if (CLI.Ins.size() > RetRegs.size()) {
    diagnoseUnsupported(DAG, DL,
                        "AVR32 multi-value call returns are not implemented "
                        "yet");
    return Chain;
  }

  for (unsigned I = 0, E = CLI.Ins.size(); I != E; ++I) {
    if (!isSupportedRegValueType(CLI.Ins[I].VT)) {
      diagnoseUnsupported(DAG, DL,
                          "AVR32 non-integer register call returns are not "
                          "implemented yet");
      InVals.push_back(DAG.getUNDEF(CLI.Ins[I].VT));
      continue;
    }

    SDValue Result = DAG.getCopyFromReg(Chain, DL, RetRegs[I], MVT::i32, Glue);
    Chain = Result.getValue(1);
    Glue = Result.getNode()->getNumValues() > 2 ? Result.getValue(2) : SDValue();
    if (CLI.Ins[I].VT != MVT::i32)
      Result = DAG.getNode(ISD::TRUNCATE, DL, CLI.Ins[I].VT, Result);
    InVals.push_back(Result);
  }

  return Chain;
}

SDValue AVR32TargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
    SelectionDAG &DAG) const {
  if (DAG.getMachineFunction().getFunction().hasFnAttribute("interrupt")) {
    if (!Outs.empty())
      diagnoseUnsupported(DAG, DL,
                          "AVR32 interrupt functions must return void");
    return DAG.getNode(AVR32ISD::RET_INTR_FLAG, DL, MVT::Other, Chain);
  }

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  ArrayRef<MCPhysReg> RetRegs = getIntArgRegs();

  if (Outs.size() > RetRegs.size()) {
    diagnoseUnsupported(DAG, DL,
                        "AVR32 multi-value returns are not implemented yet");
  }

  for (unsigned I = 0, E = std::min(Outs.size(), RetRegs.size()); I != E;
       ++I) {
    if (!isSupportedRegValueType(Outs[I].VT)) {
      diagnoseUnsupported(DAG, DL,
                          "AVR32 non-integer register returns are not "
                          "implemented yet");
      continue;
    }

    SDValue RetVal = extendToI32(OutVals[I], Outs[I].Flags, DL, DAG);
    Chain = DAG.getCopyToReg(Chain, DL, RetRegs[I], RetVal, Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(RetRegs[I], MVT::i32));
    RetOps[0] = Chain;
  }

  if (Glue)
    RetOps.push_back(Glue);
  return DAG.getNode(AVR32ISD::RET_FLAG, DL, MVT::Other, RetOps);
}
