//===-- AVR32MCTargetDesc.h - AVR32 Target Descriptions -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR32_MCTARGETDESC_AVR32MCTARGETDESC_H
#define LLVM_LIB_TARGET_AVR32_MCTARGETDESC_AVR32MCTARGETDESC_H

#include <cstdint>
#include <memory>

namespace llvm {

class MCAsmBackend;
class MCContext;
class MCFixup;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class MCTargetOptions;
class MCValue;
class Target;

MCAsmBackend *createAVR32MCAsmBackend(const Target &T,
                                      const MCSubtargetInfo &STI,
                                      const MCRegisterInfo &MRI,
                                      const MCTargetOptions &Options);

std::unique_ptr<MCObjectTargetWriter> createAVR32ELFObjectWriter(uint8_t OSABI);

} // end namespace llvm

#endif
