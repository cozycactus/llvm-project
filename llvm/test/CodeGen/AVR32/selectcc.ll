; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

define i32 @select_ugt(i32 %a, i32 %b, i32 %x) {
; CHECK-LABEL: select_ugt:
; CHECK: cp r11, r12
; CHECK-NEXT: movcs r12, r10
; CHECK-NEXT: ret r12
  %cond = icmp ugt i32 %a, %b
  %ret = select i1 %cond, i32 %x, i32 %a
  ret i32 %ret
}

define i32 @select_ugt_minsize(i32 %a, i32 %b, i32 %x) minsize optsize {
; CHECK-LABEL: select_ugt_minsize:
; CHECK: cp r11, r12
; CHECK-NEXT: brcs
; CHECK-NOT: movcs
; CHECK: ret r12
  %cond = icmp ugt i32 %a, %b
  %ret = select i1 %cond, i32 %x, i32 %a
  ret i32 %ret
}
