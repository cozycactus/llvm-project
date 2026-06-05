# RUN: not llvm-mc -triple=avr32 -filetype=obj %s -o /dev/null 2>&1 \
# RUN:   | FileCheck %s

.text
  lda.w r0, const
# CHECK: :[[@LINE-1]]:{{[0-9]+}}: error: fixup value out of range
  .space 512
const:
  .long 1
  .ltorg
