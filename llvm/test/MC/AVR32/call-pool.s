# RUN: llvm-mc -triple=avr32 -show-encoding -filetype=asm %s \
# RUN:   | FileCheck %s --check-prefix=ENCODING
# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -r -x .text %t.o | FileCheck %s --check-prefixes=RELOC,HEX

.text
  nop
  call callee
  .ltorg

# ENCODING: mcall pc[.Ltmp{{[0-9]+}}] # encoding: [0xf0{{.*}},0x1f{{.*}},A,A]
# ENCODING-NEXT: #   fixup {{.*}} - offset: 0, value: .Ltmp{{[0-9]+}}, kind: fixup_16w_pcrel
# ENCODING: .long callee

# RELOC: Relocations [
# RELOC: Section {{.*}} .rela.text {
# RELOC-NEXT: 0x8 R_AVR32_32_CPENT callee 0x0
# RELOC-NEXT: }
# RELOC-NEXT: ]

# HEX: Hex dump of section '.text':
# HEX-NEXT: 0x00000000 d703f01f 00020000 00000000
