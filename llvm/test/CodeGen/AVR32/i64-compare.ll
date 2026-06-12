; RUN: llc -mtriple=avr32 -mcpu=uc3a3256 -verify-machineinstrs < %s | FileCheck %s

target triple = "avr32"

define i32 @ult_i64(i64 %a, i64 %b) optsize {
; CHECK-LABEL: ult_i64:
; CHECK: cp r11, r9
; CHECK-NEXT: cpc r12, r10
; CHECK-NEXT: srcs
entry:
  %cmp = icmp ult i64 %a, %b
  %z = zext i1 %cmp to i32
  ret i32 %z
}

define i32 @slt_i64(i64 %a, i64 %b) optsize {
; CHECK-LABEL: slt_i64:
; CHECK: cp r11, r9
; CHECK-NEXT: cpc r12, r10
; CHECK-NEXT: srlt
entry:
  %cmp = icmp slt i64 %a, %b
  %z = zext i1 %cmp to i32
  ret i32 %z
}

declare void @side0()
declare void @side1()

define void @br_ult_i64(i64 %a, i64 %b) optsize {
; CHECK-LABEL: br_ult_i64:
; CHECK: cp r11, r9
; CHECK-NEXT: cpc r12, r10
; CHECK-NEXT: brcc
entry:
  %cmp = icmp ult i64 %a, %b
  br i1 %cmp, label %t, label %f

t:
  call void @side0()
  ret void

f:
  call void @side1()
  ret void
}

define void @br_slt_i64(i64 %a, i64 %b) optsize {
; CHECK-LABEL: br_slt_i64:
; CHECK: cp r11, r9
; CHECK-NEXT: cpc r12, r10
; CHECK-NEXT: brge
entry:
  %cmp = icmp slt i64 %a, %b
  br i1 %cmp, label %t, label %f

t:
  call void @side0()
  ret void

f:
  call void @side1()
  ret void
}
