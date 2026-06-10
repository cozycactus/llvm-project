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

define void @caller_twice() {
; CHECK-LABEL: caller_twice:
; CHECK: pushm r4-r7, lr
; CHECK: mcall pc[[CP0:\[.Ltmp[0-9]+\]]]
; CHECK-NEXT: mcall pc[[CP1:\[.Ltmp[0-9]+\]]]
; CHECK: popm r4-r7, pc
; CHECK: .long callee
; CHECK: .long callee
; RELOC: R_AVR32_32_CPENT callee 0x0
; RELOC: R_AVR32_32_CPENT callee 0x0
entry:
  call void @callee()
  call void @callee()
  ret void
}
