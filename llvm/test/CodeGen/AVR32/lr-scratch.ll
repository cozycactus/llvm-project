; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

declare void @bar()

define void @lr_inline_pressure(
    i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6,
    i32 %a7, i32 %a8, i32 %a9, i32 %a10, i32 %a11, i32 %a12, i32 %a13) {
; CHECK-LABEL: lr_inline_pressure:
; CHECK: pushm {{.*}}lr
; CHECK: mcall pc
; CHECK: lddsp lr,
; CHECK: popm {{.*}}pc
entry:
  call void @bar()
  call void asm sideeffect "", "r,r,r,r,r,r,r,r,r,r,r,r,r,r"(
      i32 %a0, i32 %a1, i32 %a2, i32 %a3, i32 %a4, i32 %a5, i32 %a6,
      i32 %a7, i32 %a8, i32 %a9, i32 %a10, i32 %a11, i32 %a12, i32 %a13)
  ret void
}
