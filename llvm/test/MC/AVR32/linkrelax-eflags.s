# RUN: llvm-mc -triple=avr32 -filetype=obj %s -o %t.o
# RUN: llvm-readobj -h %t.o | FileCheck %s --check-prefix=NORELAX
# RUN: llvm-mc -triple=avr32 -mattr=+relax -filetype=obj %s -o %t.relax.o
# RUN: llvm-readobj -h %t.relax.o | FileCheck %s --check-prefix=RELAX

.text
  nop

# NORELAX: Flags [ (0x0)
# NORELAX-NEXT: ]

# RELAX: Flags [ (0x1)
# RELAX-NEXT: EF_AVR32_LINKRELAX (0x1)
# RELAX-NEXT: ]
