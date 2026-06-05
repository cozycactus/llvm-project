//===-- AVR32TargetTransformInfo.h - AVR32 specific TTI ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR32_AVR32TARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_AVR32_AVR32TARGETTRANSFORMINFO_H

#include "AVR32Subtarget.h"
#include "AVR32TargetMachine.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/MathExtras.h"

namespace llvm {

class AVR32TTIImpl final : public BasicTTIImplBase<AVR32TTIImpl> {
  using BaseT = BasicTTIImplBase<AVR32TTIImpl>;
  using TTI = TargetTransformInfo;

  friend BaseT;

  const AVR32Subtarget *ST;
  const AVR32TargetLowering *TLI;

  const AVR32Subtarget *getST() const { return ST; }
  const AVR32TargetLowering *getTLI() const { return TLI; }

public:
  explicit AVR32TTIImpl(const AVR32TargetMachine *TM, const Function &F)
      : BaseT(TM, F.getDataLayout()), ST(TM->getSubtargetImpl(F)),
        TLI(ST->getTargetLowering()) {}

  InstructionCost getIntImmCost(const APInt &Imm, Type *Ty,
                                TTI::TargetCostKind CostKind) const override {
    assert(Ty->isIntegerTy());
    unsigned BitSize = Ty->getPrimitiveSizeInBits();
    if (BitSize == 0 || BitSize > 64)
      return TTI::TCC_Free;

    uint64_t ZExt = Imm.getZExtValue();
    int64_t SExt = Imm.getSExtValue();
    if (ZExt == 0)
      return TTI::TCC_Free;

    if (isInt<21>(SExt))
      return TTI::TCC_Basic;

    uint32_t Bits = static_cast<uint32_t>(ZExt);
    if ((Bits & 0xffff) == 0)
      return TTI::TCC_Basic;

    return 3 * TTI::TCC_Basic;
  }

  InstructionCost
  getIntImmCostInst(unsigned Opcode, unsigned Idx, const APInt &Imm, Type *Ty,
                    TTI::TargetCostKind CostKind,
                    Instruction *Inst = nullptr) const override {
    return getIntImmCost(Imm, Ty, CostKind);
  }

  InstructionCost
  getIntImmCostIntrin(Intrinsic::ID IID, unsigned Idx, const APInt &Imm,
                      Type *Ty, TTI::TargetCostKind CostKind) const override {
    return getIntImmCost(Imm, Ty, CostKind);
  }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_AVR32_AVR32TARGETTRANSFORMINFO_H
