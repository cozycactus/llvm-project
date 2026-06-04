# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -r %t.o | FileCheck %s

.text
.long 0
.reloc 0, R_AVR32_32, sym
.reloc 2, R_AVR32_16, sym+4
.reloc 0, R_AVR32_22H_PCREL, branch
.reloc 0, R_AVR32_RELATIVE
.reloc 0, R_AVR32_GLOB_DAT, ext
.reloc 0, R_AVR32_JMP_SLOT, func

# CHECK: Relocations [
# CHECK: Section {{.*}} .rela.text {
# CHECK-NEXT: 0x0 R_AVR32_32 sym 0x0
# CHECK-NEXT: 0x2 R_AVR32_16 sym 0x4
# CHECK-NEXT: 0x0 R_AVR32_22H_PCREL branch 0x0
# CHECK-NEXT: 0x0 R_AVR32_RELATIVE - 0x0
# CHECK-NEXT: 0x0 R_AVR32_GLOB_DAT ext 0x0
# CHECK-NEXT: 0x0 R_AVR32_JMP_SLOT func 0x0
# CHECK-NEXT: }
# CHECK-NEXT: ]
