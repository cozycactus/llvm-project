# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj %s -o %t.relax.o
# RUN: llvm-readobj -h -r -x .text %t.relax.o \
# RUN:   | FileCheck %s --check-prefixes=RELAX,RELAX-HEX
# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -h -r -x .text %t.o \
# RUN:   | FileCheck %s --check-prefixes=NORELAX,NORELAX-HEX

.text
  lddpc r0, pc[pool]
  .p2align 2
pool:
  .long 0

# RELAX:      Flags [ (0x1)
# RELAX-NEXT:   EF_AVR32_LINKRELAX (0x1)
# RELAX-NEXT: ]
# RELAX:      Relocations [
# RELAX-NEXT:   Section {{.*}} .rela.text {
# RELAX-NEXT:     0x0 R_AVR32_16B_PCREL .text 0x6
# RELAX-NEXT:     0x4 R_AVR32_ALIGN - 0x2
# RELAX-NEXT:   }
# RELAX-NEXT: ]

# RELAX-HEX:      Hex dump of section '.text':
# RELAX-HEX-NEXT: 0x00000000 fef00000 d7030000 0000

# NORELAX:      Flags [ (0x0)
# NORELAX-NEXT: ]
# NORELAX:      Relocations [
# NORELAX-NEXT: ]

# NORELAX-HEX:      Hex dump of section '.text':
# NORELAX-HEX-NEXT: 0x00000000 fef00004 00000000
