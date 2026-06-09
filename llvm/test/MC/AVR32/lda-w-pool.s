# RUN: llvm-mc -triple=avr32 -show-encoding -filetype=asm %s \
# RUN:   | FileCheck %s --check-prefix=ENCODING
# RUN: llvm-mc -triple=avr32 -filetype=asm %s \
# RUN:   | FileCheck %s --check-prefix=ASM
# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -r -x .text %t.o | FileCheck %s --check-prefixes=RELOC,HEX

.text
  lda.w pc, target
  lda.w r0, value
  nop
target:
  nop
  .ltorg

# ENCODING: lddpc pc, pc[.Ltmp{{[0-9]+}}] # encoding: [0x48{{.*}},0x0f{{.*}}]
# ENCODING-NEXT: #   fixup {{.*}} - offset: 0, value: .Ltmp{{[0-9]+}}, kind: fixup_7w_pcrel
# ENCODING: lddpc r0, pc[.Ltmp{{[0-9]+}}] # encoding: [0x48{{.*}},{{.*}}]
# ENCODING-NEXT: #   fixup {{.*}} - offset: 0, value: .Ltmp{{[0-9]+}}, kind: fixup_7w_pcrel
# ENCODING: .long target
# ENCODING: .long value

# ASM: lddpc pc, pc[.Ltmp{{[0-9]+}}]
# ASM: lddpc r0, pc[.Ltmp{{[0-9]+}}]
# ASM: .long target
# ASM: .long value

# RELOC: Relocations [
# RELOC: Section {{.*}} .rela.text {
# RELOC-NEXT: 0x8 R_AVR32_32_CPENT .text 0x6
# RELOC-NEXT: 0xC R_AVR32_32_CPENT value 0x0
# RELOC-NEXT: }
# RELOC-NEXT: ]

# HEX: Hex dump of section '.text':
# HEX-NEXT: 0x00000000 482f4830 d703d703 00000000 00000000
