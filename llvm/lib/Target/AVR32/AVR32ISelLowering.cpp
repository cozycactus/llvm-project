//===-- AVR32ISelLowering.cpp - AVR32 DAG lowering -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32ISelLowering.h"
#include "AVR32Subtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/DiagnosticInfo.h"

using namespace llvm;

#define GET_REGINFO_ENUM
#include "AVR32GenRegisterInfo.inc"

AVR32TargetLowering::AVR32TargetLowering(const TargetMachine &TM,
                                         const AVR32Subtarget &STI)
    : TargetLowering(TM, STI) {
  addRegisterClass(MVT::i32, &AVR32::GPRRegClass);
  computeRegisterProperties(STI.getRegisterInfo());
  setStackPointerRegisterToSaveRestore(AVR32::SP);
}

static void diagnoseUnsupported(SelectionDAG &DAG, const SDLoc &DL,
                                StringRef Message) {
  MachineFunction &MF = DAG.getMachineFunction();
  DAG.getContext()->diagnose(
      DiagnosticInfoUnsupported(MF.getFunction(), Message, DL.getDebugLoc()));
}

SDValue AVR32TargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  static const MCPhysReg ArgRegs[] = {AVR32::R12, AVR32::R11, AVR32::R10,
                                      AVR32::R9, AVR32::R8};
  if (IsVarArg || Ins.size() > std::size(ArgRegs)) {
    diagnoseUnsupported(DAG, DL,
                        "AVR32 stack and variadic arguments are not "
                        "implemented yet");
    for (const ISD::InputArg &In : Ins)
      InVals.push_back(DAG.getUNDEF(In.VT));
    return Chain;
  }

  MachineFunction &MF = DAG.getMachineFunction();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  for (unsigned I = 0, E = Ins.size(); I != E; ++I) {
    if (Ins[I].VT != MVT::i32) {
      diagnoseUnsupported(DAG, DL,
                          "AVR32 non-i32 arguments are not implemented yet");
      InVals.push_back(DAG.getUNDEF(Ins[I].VT));
      continue;
    }

    Register VReg = MRI.createVirtualRegister(&AVR32::GPRRegClass);
    MRI.addLiveIn(ArgRegs[I], VReg);
    InVals.push_back(DAG.getCopyFromReg(Chain, DL, VReg, MVT::i32));
  }
  return Chain;
}

SDValue AVR32TargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
    SelectionDAG &DAG) const {
  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);

  if (Outs.size() > 1) {
    diagnoseUnsupported(DAG, DL,
                        "AVR32 multi-value returns are not implemented yet");
  }

  if (!Outs.empty()) {
    if (Outs[0].VT != MVT::i32) {
      diagnoseUnsupported(DAG, DL,
                          "AVR32 non-i32 returns are not implemented yet");
    } else {
      Chain = DAG.getCopyToReg(Chain, DL, AVR32::R12, OutVals[0], Glue);
      Glue = Chain.getValue(1);
      RetOps.push_back(DAG.getRegister(AVR32::R12, MVT::i32));
      RetOps[0] = Chain;
    }
  }

  if (Glue)
    RetOps.push_back(Glue);
  return DAG.getNode(AVR32ISD::RET_FLAG, DL, MVT::Other, RetOps);
}
