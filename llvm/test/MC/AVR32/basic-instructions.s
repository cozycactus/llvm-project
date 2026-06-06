# RUN: llvm-mc -triple=avr32 -show-encoding -filetype=asm %s | FileCheck %s

nop
ret
rete
rets
retj
retss
breakpoint
scall
add r0, r1
sub r2, r3
mov r4, r5
moval r4, r5
pushm r0-r12, lr
popm r0-r12, lr

# CHECK: nop # encoding: [0xd7,0x03]
# CHECK-NEXT: retal lr # encoding: [0x5e,0xfe]
# CHECK-NEXT: rete # encoding: [0xd6,0x03]
# CHECK-NEXT: rets # encoding: [0xd6,0x13]
# CHECK-NEXT: retj # encoding: [0xd6,0x33]
# CHECK-NEXT: retss # encoding: [0xd7,0x63]
# CHECK-NEXT: breakpoint # encoding: [0xd6,0x73]
# CHECK-NEXT: scall # encoding: [0xd7,0x33]
# CHECK-NEXT: add r0, r1 # encoding: [0x02,0x00]
# CHECK-NEXT: sub r2, r3 # encoding: [0x06,0x12]
# CHECK-NEXT: mov r4, r5 # encoding: [0x0a,0x94]
# CHECK-NEXT: moval r4, r5 # encoding: [0xea,0x04,0x17,0xf0]
# CHECK-NEXT: pushm r0-r3, r4-r7, r8-r9, r10, r11, r12, lr # encoding: [0xd7,0xf1]
# CHECK-NEXT: popm r0-r3, r4-r7, r8-r9, r10, r11, r12, lr # encoding: [0xd7,0xf2]
