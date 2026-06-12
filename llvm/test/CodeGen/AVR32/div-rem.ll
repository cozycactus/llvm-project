; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

define i32 @sdiv_only(i32 %a, i32 %b) {
; CHECK-LABEL: sdiv_only:
; CHECK: divs [[Q:r([02468]|10)]], r12, r11
; CHECK-NEXT: mov r12, [[Q]]
; CHECK-NEXT: ret r12
  %q = sdiv i32 %a, %b
  ret i32 %q
}

define i32 @srem_only(i32 %a, i32 %b) {
; CHECK-LABEL: srem_only:
; CHECK: divs [[Q:r([02468]|10)]], r12, r11
; CHECK-NEXT: mul [[Q]], r11
; CHECK-NEXT: sub r12, [[Q]]
; CHECK-NEXT: ret r12
  %r = srem i32 %a, %b
  ret i32 %r
}

define i32 @sdiv_plus_srem(i32 %a, i32 %b) {
; CHECK-LABEL: sdiv_plus_srem:
; CHECK: divs [[Q:r([02468]|10)]], r12, r11
; CHECK-NEXT: mul [[PRODUCT:r[0-9]+]], [[Q]], r11
; CHECK-NEXT: rsub [[PRODUCT]], r12
; CHECK: add r12, [[Q]], [[PRODUCT]]
; CHECK-NEXT: ret r12
  %q = sdiv i32 %a, %b
  %r = srem i32 %a, %b
  %sum = add i32 %q, %r
  ret i32 %sum
}

define i32 @urem_only(i32 %a, i32 %b) {
; CHECK-LABEL: urem_only:
; CHECK: divu [[Q:r([02468]|10)]], r12, r11
; CHECK-NEXT: mul [[Q]], r11
; CHECK-NEXT: sub r12, [[Q]]
; CHECK-NEXT: ret r12
  %r = urem i32 %a, %b
  ret i32 %r
}

define i32 @udiv_then_use_divisor(i32 %a, i32 %b) {
; CHECK-LABEL: udiv_then_use_divisor:
; CHECK: divu [[Q:r([02468]|10)]], r12, r11
; CHECK-NEXT: add r12, [[Q]], r11
; CHECK-NEXT: ret r12
  %q = udiv i32 %a, %b
  %sum = add i32 %q, %b
  ret i32 %sum
}
