# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-dwarfdump --debug-line %t.o | FileCheck %s

.file 1 "a.s"
.text
.globl foo
foo:
.loc 1 1 0
nop
.loc 1 2 0
nop

# CHECK: min_inst_length: 2
# CHECK: Address            Line
# CHECK: 0x0000000000000000      1
# CHECK-NEXT: 0x0000000000000002      2
# CHECK-NEXT: 0x0000000000000004      2
