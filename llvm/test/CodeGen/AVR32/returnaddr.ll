; RUN: llc -mtriple=avr32 -filetype=asm < %s | FileCheck %s

declare ptr @llvm.returnaddress(i32 immarg)

define ptr @ra0() {
; CHECK-LABEL: ra0:
; CHECK: mov r12, lr
; CHECK: ret r12
  %ra = call ptr @llvm.returnaddress(i32 0)
  ret ptr %ra
}
