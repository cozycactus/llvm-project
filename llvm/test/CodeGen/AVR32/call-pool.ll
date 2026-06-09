; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avr32 -filetype=obj < %s | llvm-readobj --relocations - | FileCheck %s --check-prefix=RELOC

declare void @callee()

define void @caller() {
; CHECK-LABEL: caller:
; CHECK: pushm r4-r7, lr
; CHECK: mcall pc
; CHECK: popm r4-r7, pc
; CHECK: .long callee
; RELOC: R_AVR32_32_CPENT callee 0x0
entry:
  call void @callee()
  ret void
}
