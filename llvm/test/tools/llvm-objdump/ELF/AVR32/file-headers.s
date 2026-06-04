# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-objdump -f %t.o | FileCheck %s -DFILE=%t.o
# RUN: llvm-objdump --file-headers %t.o | FileCheck %s -DFILE=%t.o

nop

# CHECK: [[FILE]]: file format elf32-avr32
# CHECK-NEXT: architecture: avr32
# CHECK-NEXT: start address: 0x00000000
