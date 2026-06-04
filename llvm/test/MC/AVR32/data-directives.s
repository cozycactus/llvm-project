# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -x .data %t.o | FileCheck %s

.data
.short 0x1234
.long 0x12345678

# CHECK: Hex dump of section '.data':
# CHECK-NEXT: 0x00000000 12341234 5678                       .4.4Vx
