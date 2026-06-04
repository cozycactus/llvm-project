//===-- AVR32ISelLowering.cpp - AVR32 DAG lowering -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32ISelLowering.h"
#include "AVR32Subtarget.h"
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
  diagnoseUnsupported(DAG, DL,
                      "AVR32 formal argument lowering is not implemented yet");
  for (const ISD::InputArg &In : Ins)
    InVals.push_back(DAG.getUNDEF(In.VT));
  return Chain;
}

SDValue AVR32TargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
    SelectionDAG &DAG) const {
  diagnoseUnsupported(DAG, DL, "AVR32 return lowering is not implemented yet");
  return DAG.getNode(AVR32ISD::RET_FLAG, DL, MVT::Other, Chain);
}
