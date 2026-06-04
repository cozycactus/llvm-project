# RUN: not llvm-mc -triple=avr32 -filetype=obj %s -o /dev/null 2>&1 \
# RUN:   | FileCheck %s

.text
  rjmp pc[far]
# CHECK: :[[@LINE-1]]:11: error: fixup value out of range
  .space 2048
far:
  nop

  rjmp pc[odd]
# CHECK: :[[@LINE-1]]:11: error: fixup value must be 2-byte aligned
  .byte 0
odd:
  nop
