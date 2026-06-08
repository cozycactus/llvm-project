; RUN: llc -mtriple=avr32 -mcpu=uc3a3256 -verify-machineinstrs < %s | FileCheck %s

@data = global [4 x i32] zeroinitializer, align 4

define i32 @leaf_saves_regs_without_lr() minsize optsize {
; CHECK-LABEL: leaf_saves_regs_without_lr:
; CHECK: pushm r0-r3
; CHECK: popm r0-r3{{$}}
; CHECK-NEXT: ret r12
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %next, %body ]
  %acc = phi i32 [ 0, %entry ], [ %sum, %body ]
  %done = icmp eq i32 %i, 4
  br i1 %done, label %exit, label %body

body:
  %value = add nuw nsw i32 %i, 9
  %ptr = getelementptr inbounds [4 x i32], ptr @data, i32 0, i32 %i
  store volatile i32 %value, ptr %ptr, align 4
  %loaded = load volatile i32, ptr %ptr, align 4
  %sum = add nsw i32 %loaded, %acc
  %next = add nuw nsw i32 %i, 1
  br label %loop

exit:
  ret i32 %acc
}
