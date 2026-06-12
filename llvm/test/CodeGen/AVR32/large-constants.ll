; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

define i32 @large_const_default() {
; CHECK-LABEL: large_const_default:
; CHECK:      mov [[REG:r[0-9]+]], 22136
; CHECK-NEXT: orh [[REG]], 4660
; CHECK-NEXT: ret r12
  ret i32 305419896
}

define i32 @large_const_optsize() optsize {
; CHECK-LABEL: large_const_optsize:
; CHECK:      lddpc r12, pc[.Ltmp
; CHECK:      ret r12
; CHECK:      .long 305419896
  ret i32 305419896
}

define i32 @large_const_minsize() minsize optsize {
; CHECK-LABEL: large_const_minsize:
; CHECK:      lddpc r12, pc[.Ltmp
; CHECK:      ret r12
; CHECK:      .long -19088744
  ret i32 -19088744
}
