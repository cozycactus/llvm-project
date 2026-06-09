; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

define i64 @mul64(i64 %a, i64 %b) {
; CHECK-LABEL: mul64:
; CHECK-NOT: __avr32_mul64
; CHECK: mul
; CHECK: ret r12
entry:
  %m = mul i64 %a, %b
  ret i64 %m
}
