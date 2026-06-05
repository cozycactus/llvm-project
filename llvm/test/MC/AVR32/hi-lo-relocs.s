# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -r -x .text %t.o | FileCheck %s

  mov r8, LO(symbol)
  orh r8, HI(symbol)

# CHECK: Relocations [
# CHECK: Section {{.*}} .rela.text {
# CHECK-NEXT: 0x0 R_AVR32_LO16 symbol 0x0
# CHECK-NEXT: 0x4 R_AVR32_HI16 symbol 0x0
# CHECK-NEXT: }
# CHECK-NEXT: ]

# CHECK: Hex dump of section '.text':
# CHECK-NEXT: 0x00000000 e0680000 ea180000
