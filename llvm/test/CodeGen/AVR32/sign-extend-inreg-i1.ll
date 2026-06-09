; RUN: llc -mtriple=avr32 -filetype=asm < %s | FileCheck %s

define i32 @sext_i1(i32 %x) {
; CHECK-LABEL: sext_i1:
; CHECK: mov [[ONE:r[0-9]+]], 1
; CHECK: and [[BIT:r[0-9]+]], r12
; CHECK: sub r12, {{r[0-9]+}}, [[BIT]]
; CHECK: ret r12
  %b = trunc i32 %x to i1
  %s = sext i1 %b to i32
  ret i32 %s
}
