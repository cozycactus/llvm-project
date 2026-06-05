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
class MCCodeEmitter;
class MCContext;
class MCFixup;
class MCInstPrinter;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCStreamer;
class MCSubtargetInfo;
class MCTargetStreamer;
class MCTargetOptions;
class MCValue;
class Target;
class formatted_raw_ostream;

MCAsmBackend *createAVR32MCAsmBackend(const Target &T,
                                      const MCSubtargetInfo &STI,
                                      const MCRegisterInfo &MRI,
                                      const MCTargetOptions &Options);

MCCodeEmitter *createAVR32MCCodeEmitter(const MCInstrInfo &MCII,
                                        MCContext &Ctx);

std::unique_ptr<MCObjectTargetWriter> createAVR32ELFObjectWriter(uint8_t OSABI);

MCTargetStreamer *createAVR32NullTargetStreamer(MCStreamer &S);
MCTargetStreamer *createAVR32AsmTargetStreamer(MCStreamer &S,
                                               formatted_raw_ostream &OS,
                                               MCInstPrinter *InstPrint);
MCTargetStreamer *createAVR32ObjectTargetStreamer(MCStreamer &S,
                                                  const MCSubtargetInfo &STI);

} // end namespace llvm

#define GET_REGINFO_ENUM
#include "../AVR32GenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "../AVR32GenInstrInfo.inc"

#endif
