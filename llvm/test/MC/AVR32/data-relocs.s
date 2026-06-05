# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -r -x .data %t.o | FileCheck %s

.data
.byte sym
.short sym+1
.word sym+2
.long sym+3

# CHECK: Relocations [
# CHECK: Section {{.*}} .rela.data {
# CHECK-NEXT: 0x0 R_AVR32_8 sym 0x0
# CHECK-NEXT: 0x1 R_AVR32_16 sym 0x1
# CHECK-NEXT: 0x3 R_AVR32_32 sym 0x2
# CHECK-NEXT: 0x7 R_AVR32_32 sym 0x3
# CHECK-NEXT: }
# CHECK-NEXT: ]

# CHECK: Hex dump of section '.data':
# CHECK-NEXT: 0x00000000 00000000 00000000 000000
