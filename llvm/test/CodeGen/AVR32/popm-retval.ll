; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

declare void @callee()
declare i32 @use_i32(i32)

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

define i32 @setcc_call_result(i32 %a) minsize optsize {
; CHECK-LABEL: setcc_call_result:
; CHECK: pushm lr
; CHECK: rcall use_i32
; CHECK: moveq [[REG:r[0-9]+]], 1
; CHECK-NEXT: mov r12, [[REG]]
; CHECK-NEXT: popm pc{{$}}
entry:
  %x = call i32 @use_i32(i32 %a)
  %cmp = icmp eq i32 %x, 0
  %ret = zext i1 %cmp to i32
  ret i32 %ret
}
