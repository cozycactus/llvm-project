; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

define i64 @add64(i64 %a, i64 %b) {
; CHECK-LABEL: add64:
; CHECK-NOT: cp
; CHECK-NOT: srcs
; CHECK: add r11, r9
; CHECK-NEXT: adc r12, r12, r10
; CHECK-NEXT: ret r12
entry:
  %r = add i64 %a, %b
  ret i64 %r
}

define i64 @sub64(i64 %a, i64 %b) {
; CHECK-LABEL: sub64:
; CHECK-NOT: cp
; CHECK-NOT: srcs
; CHECK: sub r11, r9
; CHECK-NEXT: sbc r12, r12, r10
; CHECK-NEXT: ret r12
entry:
  %r = sub i64 %a, %b
  ret i64 %r
}
