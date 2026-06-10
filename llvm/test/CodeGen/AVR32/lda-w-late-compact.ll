; RUN: llc -mtriple=avr32 -O2 -filetype=obj -o %t.o < %s
; RUN: llvm-readobj --hex-dump=.text %t.o | FileCheck %s --check-prefix=HEX
; RUN: llc -mtriple=avr32 -O2 < %s | FileCheck %s --check-prefix=ASM

@g1 = external global i32
@g2 = external global i32

declare void @sink(ptr)

define ptr @late_lda() {
; ASM-LABEL: late_lda:
; ASM:       lddpc r12, pc[.Ltmp0]
; ASM:       lddpc r12, pc[.Ltmp{{[0-9]+}}]
;
; The first materialized address is too far from the end-of-function literal
; pool and must remain a 32-bit lddpc. The late address is close enough to use
; a compact 16-bit lddpc.
; HEX:       d421fefc {{[0-9a-f]+}}
; HEX:       48{{[0-9a-f]}}c
entry:
  call void @sink(ptr @g1)
  call void asm sideeffect "nop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop\0Anop", ""()
  ret ptr @g2
}
