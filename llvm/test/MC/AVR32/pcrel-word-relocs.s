# RUN: llvm-mc -triple=avr32 -show-encoding -filetype=asm %s \
# RUN:   | FileCheck %s --check-prefix=ENCODING
# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -r -x .text %t.o | FileCheck %s --check-prefixes=RELOC,HEX

.text
  mcall pc[callee]
  lddpc r0, pc[value]

# ENCODING: mcall pc[callee] # encoding: [0xf0'A',0x1f'A',A,A]
# ENCODING-NEXT: #   fixup A - offset: 0, value: callee, kind: fixup_16w_pcrel
# ENCODING: lddpc r0, pc[value] # encoding: [0xfe'A',0xf0'A',A,A]
# ENCODING-NEXT: #   fixup A - offset: 0, value: value, kind: fixup_16b_pcrel

# RELOC: Relocations [
# RELOC: Section {{.*}} .rela.text {
# RELOC-NEXT: 0x0 R_AVR32_18W_PCREL callee 0x0
# RELOC-NEXT: 0x4 R_AVR32_16B_PCREL value 0x0
# RELOC-NEXT: }
# RELOC-NEXT: ]

# HEX: Hex dump of section '.text':
# HEX-NEXT: 0x00000000 f01f0000 fef00000
