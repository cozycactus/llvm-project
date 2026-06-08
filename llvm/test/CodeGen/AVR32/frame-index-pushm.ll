; RUN: llc -mtriple=avr32 -mcpu=uc3a3256 -verify-machineinstrs < %s | FileCheck %s

@sink = external global i32

declare i32 @fill(ptr, i32)

define i32 @frame_index_after_pushm(i32 %x) {
; CHECK-LABEL: frame_index_after_pushm:
; CHECK: pushm lr
; CHECK-NEXT: sub sp, 12
; CHECK: sub r12, sp, 0
; CHECK-NOT: sub r12, sp, -4
entry:
  %local = alloca [3 x i32], align 4
  %p0 = getelementptr inbounds [3 x i32], ptr %local, i32 0, i32 0
  %y = call i32 @fill(ptr %p0, i32 %x)
  %v = load i32, ptr %p0, align 4
  %sum = add i32 %y, %v
  store volatile i32 %sum, ptr @sink, align 4
  ret i32 %sum
}
