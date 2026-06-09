; RUN: llc -mtriple=avr32 -filetype=asm < %s | FileCheck %s

declare void @llvm.va_start(ptr)
declare void @llvm.va_end(ptr)
declare i32 @snprintf(ptr, i32, ptr, ...)

@fmt = private unnamed_addr constant [4 x i8] c"%d\0A\00"

define i32 @first_vararg(i32 %fixed, ...) {
; CHECK-LABEL: first_vararg:
; CHECK: sub sp, 4
; CHECK: stdsp sp[0],
; CHECK: lddsp r12, sp[4]
; CHECK: sub sp, -4
; CHECK: ret r12
entry:
  %ap = alloca ptr, align 4
  call void @llvm.va_start(ptr %ap)
  %cur = load ptr, ptr %ap, align 4
  %next = getelementptr i8, ptr %cur, i32 4
  store ptr %next, ptr %ap, align 4
  %v = load i32, ptr %cur, align 4
  call void @llvm.va_end(ptr %ap)
  ret i32 %v
}

define i32 @call_snprintf(ptr %dst) {
; CHECK-LABEL: call_snprintf:
; CHECK: pushm r4-r7, lr
; CHECK: stdsp sp[0],
; CHECK: lddpc r10, pc[.Ltmp
; CHECK: mov r11, 8
; CHECK: mcall pc
; CHECK: popm r4-r7, pc
; CHECK: .long .Lfmt
; CHECK: .long snprintf
entry:
  %n = call i32 (ptr, i32, ptr, ...) @snprintf(ptr %dst, i32 8, ptr @fmt, i32 42)
  ret i32 %n
}
