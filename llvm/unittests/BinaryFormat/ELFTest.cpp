//===- ELFTest.cpp --------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/BinaryFormat/ELF.h"
#include "llvm/TargetParser/Triple.h"
#include "gtest/gtest.h"

using namespace llvm;
using namespace llvm::ELF;

namespace {
TEST(ELFTest, OSABI) {
  EXPECT_EQ(ELFOSABI_GNU, convertNameToOSABI("gnu"));
  EXPECT_EQ(ELFOSABI_FREEBSD, convertNameToOSABI("freebsd"));
  EXPECT_EQ(ELFOSABI_STANDALONE, convertNameToOSABI("standalone"));
  EXPECT_EQ(ELFOSABI_NONE, convertNameToOSABI("none"));
  // Test unrecognized strings.
  EXPECT_EQ(ELFOSABI_NONE, convertNameToOSABI(""));
  EXPECT_EQ(ELFOSABI_NONE, convertNameToOSABI("linux"));

  EXPECT_EQ("gnu", convertOSABIToName(ELFOSABI_GNU));
  EXPECT_EQ("freebsd", convertOSABIToName(ELFOSABI_FREEBSD));
  EXPECT_EQ("standalone", convertOSABIToName(ELFOSABI_STANDALONE));
  EXPECT_EQ("none", convertOSABIToName(ELFOSABI_NONE));
  // Test unrecognized values.
  EXPECT_EQ("none", convertOSABIToName(0xfe));
}

TEST(ELFTest, MachineMappings) {
  struct {
    const char *Name;
    const char *MachineName;
    uint16_t Machine;
    Triple::ArchType Arch;
  } Cases[] = {
      {"386", "386", EM_386, Triple::x86},
      {"x86_64", "x86_64", EM_X86_64, Triple::x86_64},
      {"arm", "arm", EM_ARM, Triple::arm},
      {"aarch64", "AArch64", EM_AARCH64, Triple::aarch64},
      {"avr", "avr", EM_AVR, Triple::avr},
      {"avr32", "avr32", EM_AVR32, Triple::avr32},
      {"riscv", "riscv", EM_RISCV, Triple::riscv32},
  };

  for (const auto &Case : Cases) {
    SCOPED_TRACE(Case.Name);
    EXPECT_EQ(Case.Machine, convertArchNameToEMachine(Case.Name));
    EXPECT_EQ(Case.MachineName, convertEMachineToArchName(Case.Machine));
    EXPECT_EQ(Case.Machine, convertTripleArchTypeToEMachine(Case.Arch));
  }
}
} // namespace
