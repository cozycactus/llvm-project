# RUN: rm -rf %t && split-file %s %t && cd %t

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj single.s -o single.o
# RUN: ld.lld single.o -o single
# RUN: llvm-readobj --hex-dump=.text single | FileCheck %s --check-prefix=SINGLE
# RUN: ld.lld --direct-data --emit-relocs single.o -o single-direct
# RUN: llvm-readobj --relocations --hex-dump=.text single-direct \
# RUN:   | FileCheck %s --check-prefix=DIRECT

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj pair.s -o pair.o
# RUN: ld.lld pair.o -o pair
# RUN: llvm-readobj --hex-dump=.text pair | FileCheck %s --check-prefix=PAIR
# RUN: ld.lld --emit-relocs pair.o -o pair-emit
# RUN: llvm-readobj --relocations --hex-dump=.text pair-emit \
# RUN:   | FileCheck %s --check-prefix=PAIR-EMIT
# RUN: ld.lld --no-relax pair.o -o pair-norelax
# RUN: llvm-readobj --hex-dump=.text pair-norelax \
# RUN:   | FileCheck %s --check-prefix=PAIR-NORELAX

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj pair-cpcall.s -o pair-cpcall.o
# RUN: ld.lld --emit-relocs pair-cpcall.o -o pair-cpcall
# RUN: llvm-readobj --relocations --hex-dump=.text pair-cpcall \
# RUN:   | FileCheck %s --check-prefix=PAIR-CPCALL

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj adjacent-pool.s -o adjacent-pool.o
# RUN: ld.lld --emit-relocs adjacent-pool.o -o adjacent-pool
# RUN: llvm-readobj --relocations --hex-dump=.text adjacent-pool \
# RUN:   | FileCheck %s --check-prefix=ADJACENT-POOL

# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj segmented-pool.s -o segmented-pool.o
# RUN: ld.lld --emit-relocs segmented-pool.o -o segmented-pool
# RUN: llvm-readobj --relocations --hex-dump=.text segmented-pool \
# RUN:   | FileCheck %s --check-prefix=SEGMENTED-POOL

# SINGLE:      Hex dump of section '.text':
# SINGLE-NEXT: 0x{{[0-9a-f]+}} fef80004 000110bc d703

# DIRECT:      Relocations [
# DIRECT-NEXT:   Section ({{.*}}) .rela.text {
# DIRECT-NEXT:     0x110B4 R_AVR32_21S target 0x0
# DIRECT-NEXT:     0x0 R_AVR32_NONE - 0x0
# DIRECT-NEXT:   }
# DIRECT-NEXT: ]
# DIRECT:      Hex dump of section '.text':
# DIRECT-NEXT: 0x{{[0-9a-f]+}} e07810b8 d703

# PAIR:      Hex dump of section '.text':
# PAIR-NEXT: 0x{{[0-9a-f]+}} 48184819 000110bc d703

# PAIR-EMIT:      Relocations [
# PAIR-EMIT-NEXT:   Section ({{.*}}) .rela.text {
# PAIR-EMIT-NEXT:     0x110B4 R_AVR32_9W_CP .text 0x4
# PAIR-EMIT-NEXT:     0x110B6 R_AVR32_9W_CP .text 0x4
# PAIR-EMIT-NEXT:     0x110B8 R_AVR32_32_CPENT target 0x0
# PAIR-EMIT-NEXT:   }
# PAIR-EMIT-NEXT: ]
# PAIR-EMIT:      Hex dump of section '.text':
# PAIR-EMIT-NEXT: 0x{{[0-9a-f]+}} 48184819 000110bc d703

# PAIR-NORELAX:      Hex dump of section '.text':
# PAIR-NORELAX-NEXT: 0x{{[0-9a-f]+}} fef80008 fef90004 000110c0 d703

# PAIR-CPCALL:      Relocations [
# PAIR-CPCALL-NEXT:   Section ({{.*}}) .rela.text {
# PAIR-CPCALL-NEXT:     0x110B4 R_AVR32_9W_CP .text 0x8
# PAIR-CPCALL-NEXT:     0x110B6 R_AVR32_9W_CP .text 0x8
# PAIR-CPCALL-NEXT:     0x110B8 R_AVR32_CPCALL .Lcallpool 0x0
# PAIR-CPCALL-NEXT:     0x110BC R_AVR32_32_CPENT target 0x0
# PAIR-CPCALL-NEXT:     0x110C0 R_AVR32_32_CPENT callee 0x0
# PAIR-CPCALL-NEXT:   }
# PAIR-CPCALL-NEXT: ]
# PAIR-CPCALL:      Hex dump of section '.text':
# PAIR-CPCALL-NEXT: 0x{{[0-9a-f]+}} 48284829 f01f0002 000110c4 000110c6
# PAIR-CPCALL-NEXT: 0x{{[0-9a-f]+}} d703d703

# ADJACENT-POOL:      Relocations [
# ADJACENT-POOL-NEXT:   Section ({{.*}}) .rela.text {
# ADJACENT-POOL-NEXT:     0x110B4 R_AVR32_9W_CP .text 0x8
# ADJACENT-POOL-NEXT:     0x110B6 R_AVR32_9W_CP .text 0xC
# ADJACENT-POOL-NEXT:   }
# ADJACENT-POOL-NEXT: ]
# ADJACENT-POOL:      Hex dump of section '.text':
# ADJACENT-POOL-NEXT: 0x{{[0-9a-f]+}} 48284839 d7030000 11111111 22222222

# SEGMENTED-POOL:      Relocations [
# SEGMENTED-POOL-NEXT:   Section ({{.*}}) .rela.text {
# SEGMENTED-POOL-NEXT:     0x110B4 R_AVR32_9W_CP .text 0x8
# SEGMENTED-POOL-NEXT:     0x110B6 R_AVR32_9W_CP .text 0x10
# SEGMENTED-POOL-NEXT:   }
# SEGMENTED-POOL-NEXT: ]
# SEGMENTED-POOL:      Hex dump of section '.text':
# SEGMENTED-POOL-NEXT: 0x{{[0-9a-f]+}} 48284849 d7030000 11111111 00000000
# SEGMENTED-POOL-NEXT: 0x{{[0-9a-f]+}} 22222222

#--- single.s
.text
.globl _start, target
_start:
  lddpc r8, pc[.Lpool]
.Lpool:
  .long 0
  .reloc .Lpool, R_AVR32_32_CPENT, target
target:
  nop

#--- pair-cpcall.s
.text
.globl _start, target, callee
_start:
  lddpc r8, pc[.Lpool]
  lddpc r9, pc[.Lpool]
.Lcall:
  .long 0xf01f0000
  .reloc .Lcall, R_AVR32_CPCALL, .Lcallpool
.Lpool:
  .long 0
  .reloc .Lpool, R_AVR32_32_CPENT, target
.Lcallpool:
  .long 0
  .reloc .Lcallpool, R_AVR32_32_CPENT, callee
target:
  nop
callee:
  nop

#--- pair.s
.text
.globl _start, target
_start:
  lddpc r8, pc[.Lpool]
  lddpc r9, pc[.Lpool]
.Lpool:
  .long 0
  .reloc .Lpool, R_AVR32_32_CPENT, target
target:
  nop

#--- adjacent-pool.s
.text
.globl _start
_start:
  lddpc r8, pc[.Lpool0]
  lddpc r9, pc[.Lpool1]
  nop
  .p2align 2, 0
.Lpool0:
  .long 0x11111111
.Lpool1:
  .long 0x22222222

#--- segmented-pool.s
.text
.globl _start
_start:
  lddpc r8, pc[.Lpool0]
  lddpc r9, pc[.Lpool1]
  nop
  .p2align 2, 0
.Lpool0:
  .long 0x11111111
  .long 0
.Lpool1:
  .long 0x22222222
