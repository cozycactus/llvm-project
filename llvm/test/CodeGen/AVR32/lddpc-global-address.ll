; RUN: llc -mtriple=avr32 -filetype=obj < %s -o %t.o
; RUN: llvm-readobj -x .text -r %t.o \
; RUN:   | FileCheck %s

target datalayout = "E-m:e-p:32:32-i64:32-n32-S32"
target triple = "avr32"

@g = external global i32

define ptr @addr() {
entry:
  ret ptr @g
}

; CHECK: 0x4 R_AVR32_32_CPENT g 0x0
; CHECK: 0x218 R_AVR32_32_CPENT g 0x0
; CHECK: 0x00000000 481c5efc

define ptr @addr_inline_asm_guard() {
entry:
  call void asm sideeffect ".space 520", ""()
  ret ptr @g
}

; CHECK: 0x00000210 fefc0008 5efc0000 00000000
