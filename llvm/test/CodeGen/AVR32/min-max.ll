; RUN: llc -mtriple=avr32 -mattr=+relax -O2 < %s | FileCheck %s

define i32 @smax(i32 %a, i32 %b) minsize optsize {
; CHECK-LABEL: smax:
; CHECK: max r12, r12, r11
; CHECK: ret r12
entry:
  %cmp = icmp sgt i32 %a, %b
  %sel = select i1 %cmp, i32 %a, i32 %b
  ret i32 %sel
}

define i32 @smin(i32 %a, i32 %b) minsize optsize {
; CHECK-LABEL: smin:
; CHECK: min r12, r12, r11
; CHECK: ret r12
entry:
  %cmp = icmp slt i32 %a, %b
  %sel = select i1 %cmp, i32 %a, i32 %b
  ret i32 %sel
}

define i32 @u_gt_select(i32 %a, i32 %b) minsize optsize {
; CHECK-LABEL: u_gt_select:
; CHECK-NOT: max
; CHECK: ret r12
entry:
  %cmp = icmp ugt i32 %a, %b
  %sel = select i1 %cmp, i32 %a, i32 %b
  ret i32 %sel
}

define i32 @u_lt_select(i32 %a, i32 %b) minsize optsize {
; CHECK-LABEL: u_lt_select:
; CHECK-NOT: min
; CHECK: ret r12
entry:
  %cmp = icmp ult i32 %a, %b
  %sel = select i1 %cmp, i32 %a, i32 %b
  ret i32 %sel
}
