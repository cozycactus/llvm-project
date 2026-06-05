# RUN: llvm-mc -triple=avr32 -show-encoding -filetype=asm %s \
# RUN:   | FileCheck %s --check-prefix=ENCODING
# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -x .text %t.o | FileCheck %s --check-prefix=HEX

.text
  rjmp $

# ENCODING: .Ltmp[[LABEL:[0-9]+]]:
# ENCODING-NEXT: bral .Ltmp[[LABEL]] # encoding: [0xe0'A',0x8f'A',A,A]
# ENCODING-NEXT: #   fixup A - offset: 0, value: .Ltmp[[LABEL]], kind: fixup_22h_pcrel

# HEX: Hex dump of section '.text':
# HEX-NEXT: 0x00000000 e08f0000
