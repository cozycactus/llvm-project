//===-- AVR32ISelLowering.cpp - AVR32 DAG lowering -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32ISelLowering.h"
#include "AVR32Subtarget.h"
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
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
  setOperationAction(ISD::BRCOND, MVT::Other, Expand);
  setOperationAction(ISD::BSWAP, MVT::i32, Expand);
  setOperationAction(ISD::CTLZ, MVT::i32, Legal);
  setOperationAction(ISD::CTLZ_ZERO_POISON, MVT::i32, Legal);
  setOperationAction(ISD::CTPOP, MVT::i32, Expand);
  setOperationAction(ISD::CTTZ, MVT::i32, Legal);
  setOperationAction(ISD::CTTZ_ZERO_POISON, MVT::i32, Legal);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
  setOperationAction(ISD::MULHS, MVT::i32, Expand);
  setOperationAction(ISD::MULHU, MVT::i32, Expand);
  setOperationAction(ISD::ROTL, MVT::i32, Expand);
  setOperationAction(ISD::ROTR, MVT::i32, Expand);
  setOperationAction(ISD::SELECT, MVT::i32, Custom);
  setOperationAction(ISD::SDIVREM, MVT::i32, Expand);
  setOperationAction(ISD::SREM, MVT::i32, Expand);
  setOperationAction(ISD::SMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::UDIVREM, MVT::i32, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::UREM, MVT::i32, Expand);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::i32, Custom);
  setOperationAction(ISD::SHL_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRA_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRL_PARTS, MVT::i32, Expand);
  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);
  setMinimumJumpTableEntries(UINT_MAX);
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

static void diagnoseUnsupported(SelectionDAG &DAG, const SDLoc &DL,
                                StringRef Message) {
  MachineFunction &MF = DAG.getMachineFunction();
  DAG.getContext()->diagnose(
      DiagnosticInfoUnsupported(MF.getFunction(), Message, DL.getDebugLoc()));
}

static bool isSupportedRegValueType(EVT VT) {
  return VT == MVT::i32 || VT == MVT::i16 || VT == MVT::i8;
}

static ArrayRef<MCPhysReg> getIntArgRegs() {
  static const MCPhysReg Regs[] = {AVR32::R12, AVR32::R11, AVR32::R10,
                                   AVR32::R9, AVR32::R8};
  return Regs;
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

SDValue AVR32TargetLowering::LowerOperation(SDValue Op,
                                            SelectionDAG &DAG) const {
  if (Op.getOpcode() != ISD::BR_CC && Op.getOpcode() != ISD::SELECT &&
      Op.getOpcode() != ISD::SETCC && Op.getOpcode() != ISD::SELECT_CC)
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
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(CondOperand))->get();
    SDValue LHS = Op.getOperand(0);
    SDValue RHS = Op.getOperand(1);
    if (commuteCondCodeForCompactBranch(CC))
      std::swap(LHS, RHS);
    AVR32CC::CondCodes TargetCC = intCondCodeToAVR32CC(CC);
    SDLoc DL(Op);
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

  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  SDLoc DL(Op);

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

static unsigned getMoveImmOpcodeForCC(AVR32CC::CondCodes CC) {
  switch (CC) {
  default:
    return 0;
  case AVR32CC::COND_EQ:
    return AVR32::MOVEQriCG;
  case AVR32CC::COND_NE:
    return AVR32::MOVNEriCG;
  case AVR32CC::COND_CC:
    return AVR32::MOVCCriCG;
  case AVR32CC::COND_CS:
    return AVR32::MOVCSriCG;
  case AVR32CC::COND_GE:
    return AVR32::MOVGEriCG;
  case AVR32CC::COND_LT:
    return AVR32::MOVLTriCG;
  case AVR32CC::COND_LS:
    return AVR32::MOVLSriCG;
  case AVR32CC::COND_GT:
    return AVR32::MOVGTriCG;
  case AVR32CC::COND_LE:
    return AVR32::MOVLEriCG;
  case AVR32CC::COND_HI:
    return AVR32::MOVHIriCG;
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
          MI.getOpcode() == AVR32::SELECTCCrr) &&
         "Unexpected custom inserter");

  MachineFunction *MF = BB->getParent();
  MachineRegisterInfo &MRI = MF->getRegInfo();
  const TargetInstrInfo &TII = *MF->getSubtarget().getInstrInfo();
  const DebugLoc &DL = MI.getDebugLoc();

  Register Dst = MI.getOperand(0).getReg();
  Register LHS = MI.getOperand(1).getReg();
  Register RHS = MI.getOperand(2).getReg();
  unsigned CondOperand = MI.getOpcode() == AVR32::SETCCrr ? 3 : 5;
  auto CC = static_cast<AVR32CC::CondCodes>(MI.getOperand(CondOperand).getImm());

  if (MI.getOpcode() == AVR32::SETCCrr) {
    unsigned MovOpc = getMoveImmOpcodeForCC(CC);
    if (!MovOpc)
      report_fatal_error("AVR32 condition code is not implemented yet");

    Register FalseReg = MRI.createVirtualRegister(&AVR32::GPRRegClass);
    BuildMI(*BB, MI, DL, TII.get(AVR32::CPrr)).addReg(LHS).addReg(RHS);
    BuildMI(*BB, MI, DL, TII.get(AVR32::MOVri21), FalseReg).addImm(0);
    BuildMI(*BB, MI, DL, TII.get(MovOpc), Dst).addReg(FalseReg).addImm(1);
    MI.eraseFromParent();
    return BB;
  }

  // AVR32 compact branches are often smaller than full conditional moves after
  // relaxation, especially when register allocation needs an extra copy.
  if (!MF->getFunction().hasOptSize() && !MF->getFunction().hasMinSize()) {
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

SDValue AVR32TargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  ArrayRef<MCPhysReg> ArgRegs = getIntArgRegs();
  if (IsVarArg) {
    diagnoseUnsupported(DAG, DL,
                        "AVR32 variadic arguments are not implemented yet");
    for (const ISD::InputArg &In : Ins)
      InVals.push_back(DAG.getUNDEF(In.VT));
    return Chain;
  }

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
  return Chain;
}

SDValue
AVR32TargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                               SmallVectorImpl<SDValue> &InVals) const {
  ArrayRef<MCPhysReg> ArgRegs = getIntArgRegs();
  SelectionDAG &DAG = CLI.DAG;
  SDLoc DL = CLI.DL;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  CLI.IsTailCall = false;

  unsigned StackBytes =
      CLI.Outs.size() > std::size(ArgRegs)
          ? alignTo((CLI.Outs.size() - std::size(ArgRegs)) * 4, 4)
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
    if (I < std::size(ArgRegs)) {
      RegsToPass.push_back({ArgRegs[I], Arg});
      continue;
    }

    if (!StackPtr)
      StackPtr = DAG.getCopyFromReg(Chain, DL, AVR32::SP, MVT::i32);

    unsigned Offset = (I - std::size(ArgRegs)) * 4;
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

  if (CLI.Ins.size() > ArgRegs.size()) {
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

    SDValue Result = DAG.getCopyFromReg(Chain, DL, ArgRegs[I], MVT::i32, Glue);
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
