; RUN: llc -mtriple=avr32 < %s | FileCheck %s

target datalayout = "E-m:e-p:32:32-i64:32-n32-S32"
target triple = "avr32"

@g = external global i8

define i32 @external_minus_one() {
; CHECK-LABEL: external_minus_one:
; CHECK: lddpc [[BASE:r[0-9]+]], pc[.Ltmp
; CHECK: sub r12, [[BASE]], 1
; CHECK: .long g
  ret i32 ptrtoint (ptr getelementptr (i8, ptr @g, i32 -1) to i32)
}

define i32 @external_large_offset() {
; CHECK-LABEL: external_large_offset:
; CHECK: lddpc [[BASE:r[0-9]+]], pc[.Ltmp
; CHECK: mov [[OFF:r[0-9]+]], 65535
; CHECK: orh [[OFF]], 8191
; CHECK: add r12, [[BASE]], [[OFF]]
; CHECK: .long g
  ret i32 ptrtoint (ptr getelementptr (i8, ptr @g, i32 536870911) to i32)
}
