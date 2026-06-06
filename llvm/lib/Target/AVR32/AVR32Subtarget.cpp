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

#define GET_SUBTARGETINFO_ENUM
#include "AVR32GenSubtargetInfo.inc"
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
  Info.setLibcallImpl(RTLIB::SUB_F64, RTLIB::impl___subdf3);
  Info.setLibcallImpl(RTLIB::MUL_F64, RTLIB::impl___muldf3);
  Info.setLibcallImpl(RTLIB::DIV_F64, RTLIB::impl___divdf3);
  Info.setLibcallImpl(RTLIB::FPEXT_F32_F64, RTLIB::impl___extendsfdf2);
  Info.setLibcallImpl(RTLIB::FPROUND_F64_F32, RTLIB::impl___truncdfsf2);
  Info.setLibcallImpl(RTLIB::FPTOSINT_F32_I32, RTLIB::impl___fixsfsi);
  Info.setLibcallImpl(RTLIB::FPTOSINT_F32_I64, RTLIB::impl___fixsfdi);
  Info.setLibcallImpl(RTLIB::FPTOSINT_F64_I32, RTLIB::impl___fixdfsi);
  Info.setLibcallImpl(RTLIB::FPTOSINT_F64_I64, RTLIB::impl___fixdfdi);
  Info.setLibcallImpl(RTLIB::FPTOUINT_F32_I32, RTLIB::impl___fixunssfsi);
  Info.setLibcallImpl(RTLIB::FPTOUINT_F32_I64, RTLIB::impl___fixunssfdi);
  Info.setLibcallImpl(RTLIB::FPTOUINT_F64_I32, RTLIB::impl___fixunsdfsi);
  Info.setLibcallImpl(RTLIB::FPTOUINT_F64_I64, RTLIB::impl___fixunsdfdi);
  Info.setLibcallImpl(RTLIB::SINTTOFP_I32_F32, RTLIB::impl___floatsisf);
  Info.setLibcallImpl(RTLIB::SINTTOFP_I32_F64, RTLIB::impl___floatsidf);
  Info.setLibcallImpl(RTLIB::SINTTOFP_I64_F32, RTLIB::impl___floatdisf);
  Info.setLibcallImpl(RTLIB::SINTTOFP_I64_F64, RTLIB::impl___floatdidf);
  Info.setLibcallImpl(RTLIB::UINTTOFP_I32_F32, RTLIB::impl___floatunsisf);
  Info.setLibcallImpl(RTLIB::UINTTOFP_I32_F64, RTLIB::impl___floatunsidf);
  Info.setLibcallImpl(RTLIB::UINTTOFP_I64_F32, RTLIB::impl___floatundisf);
  Info.setLibcallImpl(RTLIB::UINTTOFP_I64_F64, RTLIB::impl___floatundidf);

  Info.setLibcallImpl(RTLIB::OEQ_F32, RTLIB::impl___eqsf2);
  Info.setLibcallImpl(RTLIB::UNE_F32, RTLIB::impl___nesf2);
  Info.setLibcallImpl(RTLIB::OGE_F32, RTLIB::impl___gesf2);
  Info.setLibcallImpl(RTLIB::OLT_F32, RTLIB::impl___ltsf2);
  Info.setLibcallImpl(RTLIB::OLE_F32, RTLIB::impl___lesf2);
  Info.setLibcallImpl(RTLIB::OGT_F32, RTLIB::impl___gtsf2);
  Info.setLibcallImpl(RTLIB::UO_F32, RTLIB::impl___unordsf2);
  Info.setLibcallImpl(RTLIB::OEQ_F64, RTLIB::impl___eqdf2);
  Info.setLibcallImpl(RTLIB::UNE_F64, RTLIB::impl___nedf2);
  Info.setLibcallImpl(RTLIB::OGE_F64, RTLIB::impl___gedf2);
  Info.setLibcallImpl(RTLIB::OLT_F64, RTLIB::impl___ltdf2);
  Info.setLibcallImpl(RTLIB::OLE_F64, RTLIB::impl___ledf2);
  Info.setLibcallImpl(RTLIB::OGT_F64, RTLIB::impl___gtdf2);
  Info.setLibcallImpl(RTLIB::UO_F64, RTLIB::impl___unorddf2);

  Info.setLibcallImpl(RTLIB::MUL_I64, RTLIB::impl___muldi3);
  Info.setLibcallImpl(RTLIB::SDIV_I64, RTLIB::impl___divdi3);
  Info.setLibcallImpl(RTLIB::UDIV_I64, RTLIB::impl___udivdi3);
  Info.setLibcallImpl(RTLIB::SREM_I64, RTLIB::impl___moddi3);
  Info.setLibcallImpl(RTLIB::UREM_I64, RTLIB::impl___umoddi3);

  Info.setLibcallImpl(RTLIB::LOG_F32, RTLIB::impl_logf);
  Info.setLibcallImpl(RTLIB::LOG_F64, RTLIB::impl_log);
  Info.setLibcallImpl(RTLIB::LOG10_F32, RTLIB::impl_log10f);
  Info.setLibcallImpl(RTLIB::LOG10_F64, RTLIB::impl_log10);
}
