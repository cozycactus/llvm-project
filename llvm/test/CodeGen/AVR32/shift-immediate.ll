; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

define i32 @shl_3(i32 %x) {
; CHECK-LABEL: shl_3:
; CHECK: lsl r12, 3
; CHECK-NEXT: ret r12
entry:
  %r = shl i32 %x, 3
  ret i32 %r
}

define i32 @lshr_1(i32 %x) {
; CHECK-LABEL: lshr_1:
; CHECK: lsr r12, 1
; CHECK-NEXT: ret r12
entry:
  %r = lshr i32 %x, 1
  ret i32 %r
}

define i32 @ashr_4(i32 %x) {
; CHECK-LABEL: ashr_4:
; CHECK: asr r12, 4
; CHECK-NEXT: ret r12
entry:
  %r = ashr i32 %x, 4
  ret i32 %r
}
