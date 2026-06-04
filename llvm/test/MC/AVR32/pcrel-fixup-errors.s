# RUN: not llvm-mc -triple=avr32 -filetype=obj %s -o /dev/null 2>&1 \
# RUN:   | FileCheck %s

.text
  bral far
# CHECK: :[[@LINE-1]]:8: error: fixup value out of range
  .space 4194304
far:
  nop

  bral odd
# CHECK: :[[@LINE-1]]:8: error: fixup value must be 2-byte aligned
  .byte 0
odd:
  nop
