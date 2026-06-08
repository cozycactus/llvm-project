; RUN: llc -mtriple=avr32 -mcpu=uc3a3256 -O2 -verify-machineinstrs < %s | FileCheck %s

define i32 @seteq_i32(i32 %a, i32 %b) minsize {
; CHECK-LABEL: seteq_i32:
; CHECK:       cp r12, r11
; CHECK-NEXT:  sreq r12
; CHECK-NEXT:  ret r12
; CHECK-NOT:   moveq
entry:
  %cmp = icmp eq i32 %a, %b
  %zext = zext i1 %cmp to i32
  ret i32 %zext
}

define i32 @setult_i32(i32 %a, i32 %b) minsize {
; CHECK-LABEL: setult_i32:
; CHECK:       cp r12, r11
; CHECK-NEXT:  srcs r12
; CHECK-NEXT:  ret r12
; CHECK-NOT:   movcs
entry:
  %cmp = icmp ult i32 %a, %b
  %zext = zext i1 %cmp to i32
  ret i32 %zext
}
