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

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj mixed-half-a.s -o mixed-half-a.o
# RUN: llvm-mc -triple=avr32 -filetype=obj mixed-half-b.s -o mixed-half-b.o
# RUN: ld.lld mixed-half-a.o mixed-half-b.o -o mixed-half
# RUN: llvm-readobj --file-headers --hex-dump=.text mixed-half \
# RUN:   | FileCheck %s --check-prefix=MIXED-HALF

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

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj align-a.s -o align-a.o
# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj align-b.s -o align-b.o
# RUN: ld.lld align-a.o align-b.o -o align-relax
# RUN: llvm-readobj --hex-dump=.text align-relax \
# RUN:   | FileCheck %s --check-prefix=ALIGN

# RUN: ld.lld --no-relax align-a.o align-b.o -o align-norelax
# RUN: llvm-readobj --hex-dump=.text align-norelax \
# RUN:   | FileCheck %s --check-prefix=ALIGN-NORELAX

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj align-code-a.s -o align-code-a.o
# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj align-code-b.s -o align-code-b.o
# RUN: ld.lld align-code-a.o align-code-b.o -o align-code
# RUN: llvm-readobj --hex-dump=.text align-code \
# RUN:   | FileCheck %s --check-prefix=ALIGN-CODE

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj wordpc-align.s -o wordpc-align.o
# RUN: ld.lld wordpc-align.o -o wordpc-align
# RUN: llvm-readobj --hex-dump=.text wordpc-align \
# RUN:   | FileCheck %s --check-prefix=WORDPC-ALIGN

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj wordpc-span.s -o wordpc-span.o
# RUN: ld.lld wordpc-span.o -o wordpc-span
# RUN: llvm-readobj --hex-dump=.text wordpc-span \
# RUN:   | FileCheck %s --check-prefix=WORDPC-SPAN

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj wordpc-paired-lddpc.s -o wordpc-paired-lddpc.o
# RUN: ld.lld wordpc-paired-lddpc.o -o wordpc-paired-lddpc
# RUN: llvm-readobj --hex-dump=.text wordpc-paired-lddpc \
# RUN:   | FileCheck %s --check-prefix=WORDPC-PAIRED-LDDPC

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj vector-org.s -o vector-org.o
# RUN: echo 'SECTIONS { . = 0; .text : { *(.text) } }' > vector-org.script
# RUN: ld.lld vector-org.script vector-org.o -o vector-org
# RUN: llvm-readelf -s vector-org \
# RUN:   | FileCheck %s --check-prefix=VECTOR-ORG

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

# MIXED-HALF:      Flags [ (0x0)
# MIXED-HALF-NEXT: ]
# MIXED-HALF:      Hex dump of section '.text':
# MIXED-HALF-NEXT: 0x{{[0-9a-f]+}} e08f0004 d703d673 d703

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

# ALIGN:      Hex dump of section '.text':
# ALIGN-NEXT: 0x{{[0-9a-f]+}} c068f01f 00020000 000110c0 d703

# ALIGN-NORELAX:      Hex dump of section '.text':
# ALIGN-NORELAX-NEXT: 0x{{[0-9a-f]+}} e08f0008 f01f0002 00000000 000110c4
# ALIGN-NORELAX-NEXT: 0x{{[0-9a-f]+}} d703

# ALIGN-CODE:      Hex dump of section '.text':
# ALIGN-CODE-NEXT: 0x{{[0-9a-f]+}} e08f0004 f8000003 d703

# WORDPC-ALIGN:      Hex dump of section '.text':
# WORDPC-ALIGN-NEXT: 0x{{[0-9a-f]+}} d703e08f {{.*}}

# WORDPC-SPAN:      Hex dump of section '.text':
# WORDPC-SPAN-NEXT: 0x{{[0-9a-f]+}} f01f{{.*}} e08f{{.*}}

# WORDPC-PAIRED-LDDPC:      Hex dump of section '.text':
# WORDPC-PAIRED-LDDPC-NEXT: 0x{{[0-9a-f]+}} f01f0002 fef8000c d703fef9 00060000
# WORDPC-PAIRED-LDDPC-NEXT: 0x{{[0-9a-f]+}} 00000000

# VECTOR-ORG-DAG: 00000050 {{.*}} itlb_miss
# VECTOR-ORG-DAG: 00000060 {{.*}} dtlb_miss_read
# VECTOR-ORG-DAG: 00000070 {{.*}} dtlb_miss_write
# VECTOR-ORG-DAG: 00000074 {{.*}} tlb_miss_common
# VECTOR-ORG-DAG: 00000100 {{.*}} system_call

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

#--- mixed-half-a.s
.text
.globl _start
_start:
  bral mixed_half_target
  nop

#--- mixed-half-b.s
.text
.globl mixed_half_target
mixed_half_target:
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

#--- align-a.s
.text
.globl _start
_start:
  bral target_align
.Lcall:
  .long 0xf01f0000
  .reloc .Lcall, R_AVR32_CPCALL, .Lpool
  .short 0
  .reloc ., R_AVR32_ALIGN, 2
  .short 0
.Lpool:
  .long 0
  .reloc .Lpool, R_AVR32_32_CPENT, target_align

#--- align-b.s
.text
.globl target_align
target_align:
  nop

#--- align-code-a.s
.text
.globl _start
_start:
  bral target_code
  .reloc ., R_AVR32_ALIGN, 2
  .long 0xf8000003

#--- align-code-b.s
.text
.globl target_code
target_code:
  nop

#--- wordpc-align.s
.text
.globl _start
wordpc_target:
  nop
_start:
  bral branch_target
  mcall pc[wordpc_target]
  .reloc ., R_AVR32_ALIGN, 2
  .short 0
branch_target:
  nop

#--- wordpc-span.s
.text
.globl _start
_start:
  mcall pc[wordpc_target]
  bral branch_target
  .reloc ., R_AVR32_ALIGN, 2
  .short 0
branch_target:
  nop
wordpc_target:
  nop

#--- wordpc-paired-lddpc.s
.text
.globl _start
_start:
.Lcall:
  .long 0xf01f0000
  .reloc .Lcall, R_AVR32_18W_PCREL, .text + 8
  lddpc r8, pc[.Lpool]
wordpc_paired_target:
  nop
  lddpc r9, pc[.Lpool]
  .p2align 2, 0
.Lpool:
  .long 0

#--- vector-org.s
.text
.globl _start, itlb_miss, dtlb_miss_read, dtlb_miss_write
.globl tlb_miss_common, system_call
_start:
  bral tlb_miss_common
  .org 0x50
itlb_miss:
  pushm r0-r3
  bral tlb_miss_common
  .org 0x60
dtlb_miss_read:
  pushm r0-r3
  bral tlb_miss_common
  .org 0x70
dtlb_miss_write:
  pushm r0-r3
  .reloc ., R_AVR32_ALIGN, 2
  .short 0
tlb_miss_common:
  nop
  .org 0x100
system_call:
  pushm r12
