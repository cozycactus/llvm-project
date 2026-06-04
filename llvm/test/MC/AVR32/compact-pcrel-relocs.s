# RUN: llvm-mc -triple=avr32 -show-encoding -filetype=asm %s \
# RUN:   | FileCheck %s --check-prefix=ENCODING
# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -r -x .text %t.o | FileCheck %s --check-prefixes=RELOC,HEX

.text
  rjmp pc[local]
  rjmp pc[ext]
local:
  nop

# ENCODING: rjmp pc[local] # encoding: [0xc0'A',0x08'A']
# ENCODING-NEXT: #   fixup A - offset: 0, value: local, kind: fixup_11h_pcrel
# ENCODING: rjmp pc[ext] # encoding: [0xc0'A',0x08'A']
# ENCODING-NEXT: #   fixup A - offset: 0, value: ext, kind: fixup_11h_pcrel

# RELOC: Relocations [
# RELOC: 0x2 R_AVR32_11H_PCREL ext 0x0
# RELOC: ]

# HEX: Hex dump of section '.text':
# HEX-NEXT: 0x00000000 c028c008 d703                       .(....
