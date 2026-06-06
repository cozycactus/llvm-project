# RUN: rm -rf %t && split-file %s %t && cd %t

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj a.s -o a.o
# RUN: ld.lld --direct-data a.o -o direct
# RUN: llvm-readobj --file-headers --hex-dump=.text direct \
# RUN:   | FileCheck %s --check-prefix=DIRECT

# RUN: ld.lld --direct-data --emit-relocs a.o -o emit-relocs
# RUN: llvm-readobj --relocations --hex-dump=.text emit-relocs \
# RUN:   | FileCheck %s --check-prefixes=EMITREL,DIRECT-HEX

# RUN: ld.lld a.o -o no-direct
# RUN: llvm-readobj --hex-dump=.text no-direct \
# RUN:   | FileCheck %s --check-prefix=FULL

# RUN: ld.lld --no-relax --direct-data a.o -o no-relax
# RUN: llvm-readobj --hex-dump=.text no-relax \
# RUN:   | FileCheck %s --check-prefix=FULL

# RUN: llvm-mc -triple=avr32 -filetype=obj a.s -o noflag.o
# RUN: ld.lld --direct-data noflag.o -o noflag
# RUN: llvm-readobj --hex-dump=.text noflag \
# RUN:   | FileCheck %s --check-prefix=FULL

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj load-padded.s -o load-padded.o
# RUN: ld.lld --direct-data load-padded.o -o load-padded
# RUN: llvm-readobj --hex-dump=.text load-padded \
# RUN:   | FileCheck %s --check-prefix=LOAD-PADDED

# RUN: ld.lld --direct-data --emit-relocs load-padded.o -o load-padded-emit
# RUN: llvm-readobj --relocations --hex-dump=.text load-padded-emit \
# RUN:   | FileCheck %s --check-prefix=LOAD-PADDED-EMIT

# RUN: ld.lld load-padded.o -o load-padded-full
# RUN: llvm-readobj --hex-dump=.text load-padded-full \
# RUN:   | FileCheck %s --check-prefix=LOAD-PADDED-FULL

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj load-adjacent.s -o load-adjacent.o
# RUN: ld.lld --direct-data load-adjacent.o -o load-adjacent
# RUN: llvm-readobj --hex-dump=.text load-adjacent \
# RUN:   | FileCheck %s --check-prefix=LOAD-ADJACENT

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj load-gap.s -o load-gap.o
# RUN: ld.lld --direct-data load-gap.o -o load-gap
# RUN: llvm-readobj --hex-dump=.text load-gap \
# RUN:   | FileCheck %s --check-prefix=LOAD-GAP

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj load-labeled-gap.s -o load-labeled-gap.o
# RUN: ld.lld --direct-data load-labeled-gap.o -o load-labeled-gap
# RUN: llvm-readobj --hex-dump=.text load-labeled-gap \
# RUN:   | FileCheck %s --check-prefix=LOAD-LABELED-GAP

# DIRECT:      Flags [ (0x1)
# DIRECT-NEXT:   EF_AVR32_LINKRELAX (0x1)
# DIRECT-NEXT: ]
# DIRECT:      Hex dump of section '.text':
# DIRECT-NEXT: 0x{{[0-9a-f]+}} d703e0a0 0003d703 d7030000

# EMITREL:      Relocations [
# EMITREL-NEXT:   Section ({{.*}}) .rela.text {
# EMITREL-NEXT:     0x110B6 R_AVR32_22H_PCREL target 0x0
# EMITREL-NEXT:   }
# EMITREL-NEXT: ]

# DIRECT-HEX:      Hex dump of section '.text':
# DIRECT-HEX-NEXT: 0x{{[0-9a-f]+}} d703e0a0 0003d703 d7030000

# FULL:      Hex dump of section '.text':
# FULL-NEXT: 0x{{[0-9a-f]+}} d703f01f 0003d703 d7030000 000110bc

# LOAD-PADDED:      Hex dump of section '.text':
# LOAD-PADDED-NEXT: 0x{{[0-9a-f]+}} e07010b8 d703

# LOAD-PADDED-EMIT:      Relocations [
# LOAD-PADDED-EMIT-NEXT:   Section ({{.*}}) .rela.text {
# LOAD-PADDED-EMIT-NEXT:     0x110B4 R_AVR32_21S target 0x0
# LOAD-PADDED-EMIT-NEXT:   }
# LOAD-PADDED-EMIT-NEXT: ]
# LOAD-PADDED-EMIT:      Hex dump of section '.text':
# LOAD-PADDED-EMIT-NEXT: 0x{{[0-9a-f]+}} e07010b8 d703

# LOAD-PADDED-FULL:      Hex dump of section '.text':
# LOAD-PADDED-FULL-NEXT: 0x{{[0-9a-f]+}} 48100000 000110bc d703

# LOAD-ADJACENT:      Hex dump of section '.text':
# LOAD-ADJACENT-NEXT: 0x{{[0-9a-f]+}} d703e070 10b4

# LOAD-GAP:      Hex dump of section '.text':
# LOAD-GAP-NEXT: 0x{{[0-9a-f]+}} 4810d703 000110bc d703

# LOAD-LABELED-GAP:      Hex dump of section '.text':
# LOAD-LABELED-GAP-NEXT: 0x{{[0-9a-f]+}} 48100000 000110bc d703

#--- a.s
.text
.globl _start, target
_start:
  nop
  call target
  nop
target:
  nop
  .ltorg

#--- load-padded.s
.text
.globl _start, target
_start:
  lda.w r0, target
  .ltorg
target:
  nop

#--- load-gap.s
.text
.globl _start, target
_start:
  lda.w r0, target
  nop
  .ltorg
target:
  nop

#--- load-labeled-gap.s
.text
.globl _start, target, padding
_start:
  lda.w r0, target
padding:
  .short 0
  .ltorg
target:
  nop

#--- load-adjacent.s
.text
.globl _start, target
target:
  nop
_start:
  lda.w r0, target
  .ltorg
