//===-- AVR32 TargetInfo tests -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"
#include "gtest/gtest.h"
#include <optional>

using namespace llvm;

namespace {

TEST(AVR32TargetInfo, LookupTarget) {
  LLVMInitializeAVR32TargetInfo();

  Triple TT("avr32-unknown-unknown");
  std::string Error;
  const Target *TheTarget = TargetRegistry::lookupTarget(TT, Error);

  ASSERT_NE(TheTarget, nullptr) << Error;
  EXPECT_EQ(TheTarget->getName(), StringRef("avr32"));

  std::unique_ptr<TargetMachine> TM(TheTarget->createTargetMachine(
      TT, "", "", TargetOptions(), std::nullopt));
  EXPECT_EQ(TM.get(), nullptr);
}

} // namespace
