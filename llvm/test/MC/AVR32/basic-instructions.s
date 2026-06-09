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
asr r12, 4
lsl r12, 3
lsr r8, 1
mov r4, r5
pushm r0-r12, lr
popm r0-r12, lr
popm pc, r12=0
popm pc, r12=1
popm pc, r12=-1
popm r0-r7, pc, r12=-1

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
# CHECK-NEXT: asr r12, 4 # encoding: [0xa5,0x4c]
# CHECK-NEXT: lsl r12, 3 # encoding: [0xa3,0x7c]
# CHECK-NEXT: lsr r8, 1 # encoding: [0xa1,0x98]
# CHECK-NEXT: mov r4, r5 # encoding: [0x0a,0x94]
# CHECK-NEXT: pushm r0-r3, r4-r7, r8-r9, r10, r11, r12, lr # encoding: [0xd7,0xf1]
# CHECK-NEXT: popm r0-r3, r4-r7, r8-r9, r10, r11, r12, lr # encoding: [0xd7,0xf2]
# CHECK-NEXT: popm pc, r12=0 # encoding: [0xd8,0x0a]
# CHECK-NEXT: popm pc, r12=1 # encoding: [0xda,0x0a]
# CHECK-NEXT: popm pc, r12=-1 # encoding: [0xdc,0x0a]
# CHECK-NEXT: popm r0-r3, r4-r7, pc, r12=-1 # encoding: [0xdc,0x3a]
