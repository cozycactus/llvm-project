; RUN: llc -mtriple=avr32 -frame-pointer=all -verify-machineinstrs < %s | FileCheck %s

define i32 @leaf_with_frame_pointer(i32 %x) {
; CHECK-LABEL: leaf_with_frame_pointer:
; CHECK: pushm r4-r7, lr
; CHECK: mov r7, sp
; CHECK: mov sp, r7
; CHECK-NEXT: popm r4-r7, pc
  %y = add i32 %x, 1
  ret i32 %y
}
