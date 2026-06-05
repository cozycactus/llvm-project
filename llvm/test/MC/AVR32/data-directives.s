# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -x .data %t.o | FileCheck %s

.data
.short 0x1234
.word 0x90abcdef
.long 0x12345678

# CHECK: Hex dump of section '.data':
# CHECK-NEXT: 0x00000000 123490ab cdef1234 5678              .4.....4Vx
