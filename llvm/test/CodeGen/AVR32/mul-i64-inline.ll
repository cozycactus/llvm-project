; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

define i64 @umul32(i32 %a, i32 %b) {
; CHECK-LABEL: umul32:
; CHECK-NOT: castu.h
; CHECK-NOT: lsr
; CHECK: mulu.d [[PAIR:r([02468]|10)]], r12, r11
; CHECK-NOT: castu.h
; CHECK-NOT: lsr
; CHECK: ret r12
entry:
  %az = zext i32 %a to i64
  %bz = zext i32 %b to i64
  %m = mul i64 %az, %bz
  ret i64 %m
}

define i64 @mul64(i64 %a, i64 %b) {
; CHECK-LABEL: mul64:
; CHECK-NOT: __avr32_mul64
; CHECK: mul
; CHECK: ret r12
entry:
  %m = mul i64 %a, %b
  ret i64 %m
}
