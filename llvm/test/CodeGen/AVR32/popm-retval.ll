; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

declare void @callee()

define i32 @ret0_after_call() {
; CHECK-LABEL: ret0_after_call:
; CHECK: pushm lr
; CHECK: rcall callee
; CHECK-NEXT: popm pc, r12=0
entry:
  call void @callee()
  ret i32 0
}

define i32 @ret1_after_call() {
; CHECK-LABEL: ret1_after_call:
; CHECK: pushm lr
; CHECK: rcall callee
; CHECK-NEXT: popm pc, r12=1
entry:
  call void @callee()
  ret i32 1
}

define i32 @retm1_after_call() {
; CHECK-LABEL: retm1_after_call:
; CHECK: pushm lr
; CHECK: rcall callee
; CHECK-NEXT: popm pc, r12=-1
entry:
  call void @callee()
  ret i32 -1
}

define i32 @ret2_after_call() {
; CHECK-LABEL: ret2_after_call:
; CHECK: pushm lr
; CHECK: rcall callee
; CHECK-NEXT: mov r12, 2
; CHECK-NEXT: popm pc{{$}}
entry:
  call void @callee()
  ret i32 2
}
