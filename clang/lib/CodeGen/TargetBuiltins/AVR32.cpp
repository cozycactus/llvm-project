//===--- AVR32.cpp - Emit LLVM Code for AVR32 builtins --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CodeGenFunction.h"
#include "clang/Basic/TargetBuiltins.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Intrinsics.h"

using namespace clang;
using namespace CodeGen;

llvm::Value *CodeGenFunction::EmitAVR32BuiltinExpr(unsigned BuiltinID,
                                                   const CallExpr *E) {
  switch (BuiltinID) {
  default:
    return nullptr;
  case AVR32::BI__builtin_bswap_16: {
    llvm::Value *Value = EmitScalarExpr(E->getArg(0));
    llvm::Function *F = CGM.getIntrinsic(llvm::Intrinsic::bswap, Int16Ty);
    return Builder.CreateCall(F, Value);
  }
  case AVR32::BI__builtin_mfsr: {
    llvm::Value *SysReg = EmitScalarExpr(E->getArg(0));
    llvm::FunctionType *FTy =
        llvm::FunctionType::get(Int32Ty, {Int32Ty}, false);
    llvm::InlineAsm *IA = llvm::InlineAsm::get(
        FTy, "mfsr $0, $1", "=r,i", /*hasSideEffects=*/true);
    return Builder.CreateCall(IA, {SysReg});
  }
  case AVR32::BI__builtin_mtsr: {
    llvm::Value *SysReg = EmitScalarExpr(E->getArg(0));
    llvm::Value *Value = EmitScalarExpr(E->getArg(1));
    llvm::FunctionType *FTy =
        llvm::FunctionType::get(VoidTy, {Int32Ty, Int32Ty}, false);
    llvm::InlineAsm *IA = llvm::InlineAsm::get(
        FTy, "mtsr $0, $1", "i,r", /*hasSideEffects=*/true);
    return Builder.CreateCall(IA, {SysReg, Value});
  }
  }
}
