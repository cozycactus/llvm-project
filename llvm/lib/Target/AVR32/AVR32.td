//===-- AVR32.td - Describe the AVR32 Target Machine -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

include "AVR32RegisterInfo.td"
include "AVR32InstrInfo.td"

def FeatureRelax : SubtargetFeature<"relax", "EnableLinkerRelax", "true",
                                    "Enable linker relaxation">;
def FeatureCondStore
    : SubtargetFeature<"cond-store", "HasCondStore", "true",
                       "Enable conditional store instructions">;

defm : RemapAllTargetPseudoPointerOperands<GPR>;

def AVR32InstrInfo : InstrInfo;

def : Processor<"generic", NoItineraries, []>;
def : Processor<"ap", NoItineraries, []>;
def : Processor<"uc3a3128", NoItineraries, [FeatureCondStore]>;
def : Processor<"uc3a3256", NoItineraries, [FeatureCondStore]>;
def : Processor<"uc3a3256s", NoItineraries, [FeatureCondStore]>;
def : Processor<"uc3a3revd", NoItineraries, [FeatureCondStore]>;

def AVR32InstPrinter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
  bit isMCAsmWriter = 1;
}

def AVR32 : Target {
  let InstructionSet = AVR32InstrInfo;
  let AssemblyWriters = [AVR32InstPrinter];
}
