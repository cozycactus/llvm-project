# RUN: not llvm-mc -triple=avr32 -filetype=obj %s -o /dev/null 2>&1 \
# RUN:   | FileCheck %s

.text
  rjmp $+2097152
# CHECK: :[[@LINE-1]]:9: error: fixup value out of range

  rjmp $+1
# CHECK: :[[@LINE-1]]:9: error: fixup value must be 2-byte aligned
