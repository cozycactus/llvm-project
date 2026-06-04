//===-- AVR32Subtarget.cpp - AVR32 subtarget information -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AVR32Subtarget.h"
#include "llvm/CodeGen/LibcallLoweringInfo.h"

using namespace llvm;

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AVR32GenSubtargetInfo.inc"

void AVR32Subtarget::anchor() {}

AVR32Subtarget &
AVR32Subtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS) {
  StringRef CPUName = CPU.empty() ? "generic" : CPU;
  ParseSubtargetFeatures(CPUName, /*TuneCPU*/ CPUName, FS);
  return *this;
}

AVR32Subtarget::AVR32Subtarget(const Triple &TT, const std::string &CPU,
                               const std::string &FS, const TargetMachine &TM)
    : AVR32GenSubtargetInfo(TT, CPU, /*TuneCPU*/ CPU, FS),
      InstrInfo(initializeSubtargetDependencies(CPU, FS)), FrameLowering(*this),
      TLInfo(TM, *this), TSInfo() {}

void AVR32Subtarget::initLibcallLoweringInfo(
    LibcallLoweringInfo &Info) const {
  Info.setLibcallImpl(RTLIB::MEMCPY, RTLIB::impl_memcpy);
  Info.setLibcallImpl(RTLIB::MEMMOVE, RTLIB::impl_memmove);
  Info.setLibcallImpl(RTLIB::MEMSET, RTLIB::impl_memset);

  Info.setLibcallImpl(RTLIB::ADD_F32, RTLIB::impl___addsf3);
  Info.setLibcallImpl(RTLIB::SUB_F32, RTLIB::impl___subsf3);
  Info.setLibcallImpl(RTLIB::MUL_F32, RTLIB::impl___mulsf3);
  Info.setLibcallImpl(RTLIB::DIV_F32, RTLIB::impl___divsf3);
  Info.setLibcallImpl(RTLIB::ADD_F64, RTLIB::impl___adddf3);
  Info.setLibcallImpl(RTLIB::MUL_F64, RTLIB::impl___muldf3);
  Info.setLibcallImpl(RTLIB::FPEXT_F32_F64, RTLIB::impl___extendsfdf2);
  Info.setLibcallImpl(RTLIB::FPROUND_F64_F32, RTLIB::impl___truncdfsf2);
  Info.setLibcallImpl(RTLIB::FPTOSINT_F32_I32, RTLIB::impl___fixsfsi);
  Info.setLibcallImpl(RTLIB::FPTOSINT_F64_I32, RTLIB::impl___fixdfsi);
  Info.setLibcallImpl(RTLIB::SINTTOFP_I32_F32, RTLIB::impl___floatsisf);
  Info.setLibcallImpl(RTLIB::SINTTOFP_I32_F64, RTLIB::impl___floatsidf);
  Info.setLibcallImpl(RTLIB::UINTTOFP_I32_F32, RTLIB::impl___floatunsisf);
  Info.setLibcallImpl(RTLIB::UINTTOFP_I32_F64, RTLIB::impl___floatunsidf);
}
