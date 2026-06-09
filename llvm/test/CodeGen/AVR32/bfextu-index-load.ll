; RUN: llc -mtriple=avr32 -mcpu=uc3a3256 -O2 -verify-machineinstrs < %s | FileCheck %s

@table = external global [128 x i32], align 4

define i32 @fold_shifted_mask_global_index(i32 %x) minsize {
; CHECK-LABEL: fold_shifted_mask_global_index:
; CHECK:      lsr r12, 11
; CHECK:      and [[IDX:r[0-9]+]], r12, {{r[0-9]+}}
; CHECK:      lddpc [[BASE:r[0-9]+]], pc[.Ltmp
; CHECK:      ld.w r12, [[BASE]][[[IDX]] << 0]
; CHECK:      ret r12
entry:
  %shifted = lshr i32 %x, 11
  %masked = and i32 %shifted, 28
  %index = lshr exact i32 %masked, 2
  %ptr = getelementptr inbounds [128 x i32], ptr @table, i32 0, i32 %index
  %value = load i32, ptr %ptr, align 4
  ret i32 %value
}
