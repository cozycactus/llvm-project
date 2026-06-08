; RUN: opt -mtriple=avr32 -passes=loop-unroll -S < %s | FileCheck %s

target datalayout = "E-m:e-p:32:32-i64:32-n32-S32"
target triple = "avr32"

define i32 @sum8(ptr %p) {
; CHECK-LABEL: define i32 @sum8(
; CHECK: loop:
; CHECK: phi i32
; CHECK: br i1 %done, label %exit, label %loop
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inc, %loop ]
  %sum = phi i32 [ 0, %entry ], [ %add, %loop ]
  %gep = getelementptr i32, ptr %p, i32 %i
  %v = load i32, ptr %gep, align 4
  %add = add i32 %sum, %v
  %inc = add i32 %i, 1
  %done = icmp eq i32 %inc, 8
  br i1 %done, label %exit, label %loop

exit:
  ret i32 %add
}
