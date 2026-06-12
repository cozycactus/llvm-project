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

define i32 @eq_zero_i64(i64 %a) optsize {
; CHECK-LABEL: eq_zero_i64:
; CHECK: mov r8, 0
; CHECK-NEXT: cp r11, r8
; CHECK-NEXT: cpc r12, r8
; CHECK-NEXT: sreq
entry:
  %cmp = icmp eq i64 %a, 0
  %z = zext i1 %cmp to i32
  ret i32 %z
}

define i32 @ne_zero_i64(i64 %a) optsize {
; CHECK-LABEL: ne_zero_i64:
; CHECK: mov r8, 0
; CHECK-NEXT: cp r11, r8
; CHECK-NEXT: cpc r12, r8
; CHECK-NEXT: srne
entry:
  %cmp = icmp ne i64 %a, 0
  %z = zext i1 %cmp to i32
  ret i32 %z
}

define i32 @sge_zero_i64(i64 %a) optsize {
; CHECK-LABEL: sge_zero_i64:
; CHECK: mov r8, -1
; CHECK-NEXT: cp r8, r11
; CHECK-NEXT: cpc r8, r12
; CHECK-NEXT: srlt
entry:
  %cmp = icmp sge i64 %a, 0
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

define void @br_eq_zero_i64(i64 %a) optsize {
; CHECK-LABEL: br_eq_zero_i64:
; CHECK: mov r8, 0
; CHECK-NEXT: cp r11, 0
; CHECK-NEXT: cpc r12, r8
; CHECK-NEXT: breq
entry:
  %cmp = icmp eq i64 %a, 0
  br i1 %cmp, label %t, label %f

t:
  call void @side0()
  ret void

f:
  call void @side1()
  ret void
}

define void @br_slt_zero_i64(i64 %a) optsize {
; CHECK-LABEL: br_slt_zero_i64:
; CHECK: mov r8, -1
; CHECK-NEXT: cp r8, r11
; CHECK-NEXT: cpc r8, r12
; CHECK-NEXT: brge
entry:
  %cmp = icmp slt i64 %a, 0
  br i1 %cmp, label %t, label %f

t:
  call void @side0()
  ret void

f:
  call void @side1()
  ret void
}

define void @br_sge_zero_i64(i64 %a) optsize {
; CHECK-LABEL: br_sge_zero_i64:
; CHECK: mov r8, 0
; CHECK-NEXT: cp r11, 0
; CHECK-NEXT: cpc r12, r8
; CHECK-NEXT: brlt
entry:
  %cmp = icmp sge i64 %a, 0
  br i1 %cmp, label %t, label %f

t:
  call void @side0()
  ret void

f:
  call void @side1()
  ret void
}
