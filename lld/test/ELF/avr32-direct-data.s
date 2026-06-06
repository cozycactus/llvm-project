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
