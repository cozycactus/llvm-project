// RUN: %clang_cc1 -E -dM -triple avr32 -target-cpu uc3a3128 /dev/null | FileCheck -match-full-lines --check-prefixes=UC3A,UC3A3128 %s
// RUN: %clang_cc1 -E -dM -triple avr32 -target-cpu uc3a3256 /dev/null | FileCheck -match-full-lines --check-prefixes=UC3A,UC3A3256 %s
// RUN: %clang_cc1 -E -dM -triple avr32 -target-cpu uc3a3revd /dev/null | FileCheck -match-full-lines --check-prefixes=UC3A,UC3A3REVD %s
// RUN: %clang_cc1 -E -dM -triple avr32 -target-cpu ap /dev/null | FileCheck -match-full-lines --check-prefix=AP %s
// RUN: %clang --target=avr32 -march=ap -E -dM -x c /dev/null | FileCheck -match-full-lines --check-prefix=AP %s
// RUN: %clang --target=avr32 -mcpu=uc3a3128 -E -dM -x c /dev/null | FileCheck -match-full-lines --check-prefix=DRIVER %s

// AP: #define __AVR32_AP__ 1
// AP: #define __AVR32_AVR32B__ 1
// AP: #define __AVR32_ELF__ 1
// AP: #define __AVR32_HAS_BRANCH_PRED__ 1
// AP: #define __AVR32_HAS_DSP__ 1
// AP: #define __AVR32_HAS_SIMD__ 1
// AP: #define __AVR32_HAS_UNALIGNED_WORD__ 1
// AP: #define __AVR32__ 1
// AP: #define __ELF__ 1
// AP: #define __REGISTER_PREFIX__
// AP: #define __WINT_TYPE__ unsigned int
// AP: #define __avr32__ 1

// UC3A: #define __AVR32_AVR32A__ 1
// UC3A: #define __AVR32_ELF__ 1
// UC3A: #define __AVR32_HAS_DSP__ 1
// UC3A: #define __AVR32_HAS_RMW__ 1

// UC3A3128: #define __AVR32_UC3A3128__ 1
// UC3A3256: #define __AVR32_UC3A3256__ 1
// UC3A3REVD: #define __AVR32_NO_MUL__ 1
// UC3A3REVD: #define __AVR32_UC3A3256S__ 1
// UC3A: #define __AVR32_UC__ 2
// UC3A: #define __AVR32__ 1
// UC3A: #define __ELF__ 1
// UC3A: #define __REGISTER_PREFIX__
// UC3A: #define __WINT_TYPE__ unsigned int
// UC3A: #define __avr32__ 1

// DRIVER: #define __CHAR_UNSIGNED__ 1
// DRIVER: #define __WINT_TYPE__ unsigned int
