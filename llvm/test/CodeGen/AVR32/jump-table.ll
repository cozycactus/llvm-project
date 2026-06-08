; RUN: llc -mtriple=avr32 -O2 -verify-machineinstrs < %s | FileCheck %s

define i32 @dense_switch(i32 %x) {
; CHECK-LABEL: dense_switch:
; CHECK:       lddpc [[BASE:r[0-9]+]], pc[.Ltmp
; CHECK-NEXT:  ld.w pc, [[BASE]][r12 << 2]
; CHECK:       .long CPENT(.LJTI
; CHECK:       .section .rodata
; CHECK:       .LJTI
entry:
  switch i32 %x, label %default [
    i32 0, label %case0
    i32 1, label %case1
    i32 2, label %case2
    i32 3, label %case3
    i32 4, label %case4
    i32 5, label %case5
    i32 6, label %case6
    i32 7, label %case7
  ]

case0:
  ret i32 10

case1:
  ret i32 11

case2:
  ret i32 12

case3:
  ret i32 13

case4:
  ret i32 14

case5:
  ret i32 15

case6:
  ret i32 16

case7:
  ret i32 17

default:
  ret i32 99
}
