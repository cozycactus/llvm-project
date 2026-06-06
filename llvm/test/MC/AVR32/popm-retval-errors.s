# RUN: not llvm-mc -triple=avr32 -filetype=asm %s 2>&1 | FileCheck %s

popm lr, pc, r12=0
# CHECK: :[[@LINE-1]]:14: error: can't pop LR or R12 when specifying return value

popm pc, r12=2
# CHECK: :[[@LINE-1]]:14: error: invalid popm return value

popm r0-r3, r12=1
# CHECK: :[[@LINE-1]]:13: error: return value requires pc in popm register list
