# RUN: rm -rf %t && split-file %s %t && cd %t

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj a.s -o a.o
# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj b.s -o b.o
# RUN: ld.lld a.o b.o -o relax
# RUN: llvm-readobj --file-headers --hex-dump=.text relax \
# RUN:   | FileCheck %s --check-prefix=RELAX

# RUN: ld.lld --no-relax a.o b.o -o no-relax
# RUN: llvm-readobj --file-headers --hex-dump=.text no-relax \
# RUN:   | FileCheck %s --check-prefixes=NORELAX,FULL

# RUN: llvm-mc -triple=avr32 -filetype=obj a.s -o a.noflag.o
# RUN: llvm-mc -triple=avr32 -filetype=obj b.s -o b.noflag.o
# RUN: ld.lld a.noflag.o b.noflag.o -o noflag
# RUN: llvm-readobj --file-headers --hex-dump=.text noflag \
# RUN:   | FileCheck %s --check-prefixes=NOFLAG,FULL

# RUN: ld.lld --emit-relocs a.o b.o -o emit-relocs
# RUN: llvm-readobj --relocations emit-relocs \
# RUN:   | FileCheck %s --check-prefix=EMITREL

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj call-a.s -o call-a.o
# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj call-b.s -o call-b.o
# RUN: ld.lld call-a.o call-b.o -o call-relax
# RUN: llvm-readobj --hex-dump=.text call-relax \
# RUN:   | FileCheck %s --check-prefix=CALL

# RUN: ld.lld --emit-relocs call-a.o call-b.o -o call-emit-relocs
# RUN: llvm-readobj --relocations call-emit-relocs \
# RUN:   | FileCheck %s --check-prefix=CALL-EMITREL

# RELAX:      Flags [ (0x1)
# RELAX-NEXT:   EF_AVR32_LINKRELAX (0x1)
# RELAX-NEXT: ]
# RELAX:      Hex dump of section '.text':
# RELAX-NEXT: 0x{{[0-9a-f]+}} c0e8c0e0 c0e1c0e2 c0e3c0e4 c0e5c0e6
# RELAX-NEXT: 0x{{[0-9a-f]+}} c0e7e088 000ee089 000dd673 d703d703
# RELAX-NEXT: 0x{{[0-9a-f]+}} d703d703 d703d703 d703d703 d703d703
# RELAX-NEXT: 0x{{[0-9a-f]+}} d703

# NORELAX:      Flags [ (0x1)
# NORELAX-NEXT:   EF_AVR32_LINKRELAX (0x1)
# NORELAX-NEXT: ]

# NOFLAG:      Flags [ (0x0)
# NOFLAG-NEXT: ]

# FULL:      Hex dump of section '.text':
# FULL-NEXT: 0x{{[0-9a-f]+}} e08f0016 e0800015 e0810014 e0820013
# FULL-NEXT: 0x{{[0-9a-f]+}} e0830012 e0840011 e0850010 e086000f
# FULL-NEXT: 0x{{[0-9a-f]+}} e087000e e088000d e089000c d703d703
# FULL-NEXT: 0x{{[0-9a-f]+}} d703d703 d703d703 d703d703 d703d703
# FULL-NEXT: 0x{{[0-9a-f]+}} d703

# EMITREL:      Relocations [
# EMITREL-NEXT:   Section ({{.*}}) .rela.text {
# EMITREL-NEXT:     0x110B4 R_AVR32_11H_PCREL target_uncond 0x0
# EMITREL-NEXT:     0x110B6 R_AVR32_9H_PCREL target_eq 0x0
# EMITREL-NEXT:     0x110B8 R_AVR32_9H_PCREL target_ne 0x0
# EMITREL-NEXT:     0x110BA R_AVR32_9H_PCREL target_cc 0x0
# EMITREL-NEXT:     0x110BC R_AVR32_9H_PCREL target_cs 0x0
# EMITREL-NEXT:     0x110BE R_AVR32_9H_PCREL target_ge 0x0
# EMITREL-NEXT:     0x110C0 R_AVR32_9H_PCREL target_lt 0x0
# EMITREL-NEXT:     0x110C2 R_AVR32_9H_PCREL target_mi 0x0
# EMITREL-NEXT:     0x110C4 R_AVR32_9H_PCREL target_pl 0x0
# EMITREL-NEXT:     0x110C6 R_AVR32_22H_PCREL target_ls 0x0
# EMITREL-NEXT:     0x110CA R_AVR32_22H_PCREL target_gt 0x0
# EMITREL-NEXT:   }
# EMITREL-NEXT: ]

# CALL:      Hex dump of section '.text':
# CALL-NEXT: 0x{{[0-9a-f]+}} c02cd703 d703

# CALL-EMITREL:      Relocations [
# CALL-EMITREL-NEXT:   Section ({{.*}}) .rela.text {
# CALL-EMITREL-NEXT:     0x110B4 R_AVR32_11H_PCREL target_call 0x0
# CALL-EMITREL-NEXT:   }
# CALL-EMITREL-NEXT: ]

#--- a.s
.text
.globl _start
_start:
  bral target_uncond
  breq target_eq
  brne target_ne
  brcc target_cc
  brcs target_cs
  brge target_ge
  brlt target_lt
  brmi target_mi
  brpl target_pl
  brls target_ls
  brgt target_gt

#--- b.s
.text
.globl target_uncond, target_eq, target_ne, target_cc, target_cs
.globl target_ge, target_lt, target_mi, target_pl, target_ls, target_gt
target_uncond:
  nop
target_eq:
  nop
target_ne:
  nop
target_cc:
  nop
target_cs:
  nop
target_ge:
  nop
target_lt:
  nop
target_mi:
  nop
target_pl:
  nop
target_ls:
  nop
target_gt:
  nop

#--- call-a.s
.text
.globl _start
_start:
  rcall target_call
  nop

#--- call-b.s
.text
.globl target_call
target_call:
  nop
