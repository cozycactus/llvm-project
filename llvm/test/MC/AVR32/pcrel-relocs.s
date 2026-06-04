# RUN: llvm-mc -triple=avr32 -show-encoding -filetype=asm %s \
# RUN:   | FileCheck %s --check-prefix=ENCODING
# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -r -x .text %t.o | FileCheck %s --check-prefixes=RELOC,HEX

local:
  bral local
  breq local
  rcall pc[ext_call]
  bral ext_branch
  breq ext_eq

# ENCODING: bral local # encoding: [0xe0'A',0x8f'A',A,A]
# ENCODING-NEXT: #   fixup A - offset: 0, value: local, kind: fixup_22h_pcrel
# ENCODING: breq local # encoding: [0xe0'A',0x80'A',A,A]
# ENCODING-NEXT: #   fixup A - offset: 0, value: local, kind: fixup_22h_pcrel
# ENCODING: rcall pc[ext_call] # encoding: [0xe0'A',0xa0'A',A,A]
# ENCODING-NEXT: #   fixup A - offset: 0, value: ext_call, kind: fixup_22h_pcrel
# ENCODING: bral ext_branch # encoding: [0xe0'A',0x8f'A',A,A]
# ENCODING-NEXT: #   fixup A - offset: 0, value: ext_branch, kind: fixup_22h_pcrel
# ENCODING: breq ext_eq # encoding: [0xe0'A',0x80'A',A,A]
# ENCODING-NEXT: #   fixup A - offset: 0, value: ext_eq, kind: fixup_22h_pcrel

# RELOC: Relocations [
# RELOC: 0x8 R_AVR32_22H_PCREL ext_call 0x0
# RELOC: 0xC R_AVR32_22H_PCREL ext_branch 0x0
# RELOC: 0x10 R_AVR32_22H_PCREL ext_eq 0x0
# RELOC: ]

# HEX: Hex dump of section '.text':
# HEX-NEXT: 0x00000000 e08f0000 fe90fffe e0a00000 e08f0000
# HEX-NEXT: 0x00000010 e0800000
