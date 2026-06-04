# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -r --unwind %t.o 2>&1 \
# RUN:   | FileCheck %s --implicit-check-not=warning

.text
.globl foo
.type foo,@function
foo:
.cfi_startproc
.cfi_def_cfa r7, 0
.cfi_offset lr, -4
nop
.cfi_endproc

# CHECK: Relocations [
# CHECK: 0x1C R_AVR32_32_PCREL .text 0x0
# CHECK: ]
# CHECK: return_address_register: 14
# CHECK: initial_location: 0x0
# CHECK-NEXT: address_range: 0x2 (end : 0x2)
# CHECK: DW_CFA_def_cfa: reg7 +0
# CHECK-NEXT: DW_CFA_offset: reg14 -4
