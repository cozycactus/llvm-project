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

define i32 @stack_args_after_pushm(i32 %a, i32 %b, i32 %c, i32 %d,
                                   i32 %e, i32 %f, i32 %g, i32 %h) {
; CHECK-LABEL: stack_args_after_pushm:
; CHECK: pushm r0-r3
; CHECK-NOT: lddsp {{r[0-9]+}}, sp[0]
; CHECK: lddsp {{r[0-9]+}}, sp[16]
; CHECK: lddsp {{r[0-9]+}}, sp[20]
; CHECK: lddsp {{r[0-9]+}}, sp[24]
; CHECK: popm r0-r3
entry:
  call void asm sideeffect "", "~{r0},~{r1},~{r2},~{r3}"()
  %ab = add i32 %a, %b
  %abc = add i32 %ab, %c
  %abcd = add i32 %abc, %d
  %abcde = add i32 %abcd, %e
  %with_f = add i32 %abcde, %f
  %with_g = add i32 %with_f, %g
  %with_h = add i32 %with_g, %h
  %twice_f = shl i32 %f, 1
  %sum = add i32 %with_h, %twice_f
  ret i32 %sum
}
