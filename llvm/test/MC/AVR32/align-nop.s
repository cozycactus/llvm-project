# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -x .text %t.o | FileCheck %s

nop
  .p2align 2
nop

# CHECK: Hex dump of section '.text':
# CHECK-NEXT: 0x00000000 d703d703 d703                       ......
