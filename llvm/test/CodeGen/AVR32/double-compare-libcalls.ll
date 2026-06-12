; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

define i32 @oeq(double %a, double %b) minsize optsize {
; CHECK-LABEL: oeq:
; CHECK: mcall pc
; CHECK: cp r12, 0
; CHECK-NEXT: srne r12
; CHECK: .long __avr32_f64_cmp_eq
entry:
  %cmp = fcmp oeq double %a, %b
  %ret = zext i1 %cmp to i32
  ret i32 %ret
}

define i32 @oge(double %a, double %b) minsize optsize {
; CHECK-LABEL: oge:
; CHECK: mcall pc
; CHECK: cp r12, 0
; CHECK-NEXT: srne r12
; CHECK: .long __avr32_f64_cmp_ge
entry:
  %cmp = fcmp oge double %a, %b
  %ret = zext i1 %cmp to i32
  ret i32 %ret
}

define i32 @olt(double %a, double %b) minsize optsize {
; CHECK-LABEL: olt:
; CHECK: mcall pc
; CHECK: cp r12, 0
; CHECK-NEXT: srne r12
; CHECK: .long __avr32_f64_cmp_lt
entry:
  %cmp = fcmp olt double %a, %b
  %ret = zext i1 %cmp to i32
  ret i32 %ret
}

define i32 @ogt(double %a, double %b) minsize optsize {
; CHECK-LABEL: ogt:
; CHECK: mcall pc
; CHECK: cp r12, 0
; CHECK-NEXT: srne r12
; CHECK: .long __avr32_f64_cmp_lt
entry:
  %cmp = fcmp ogt double %a, %b
  %ret = zext i1 %cmp to i32
  ret i32 %ret
}

define i32 @ole(double %a, double %b) minsize optsize {
; CHECK-LABEL: ole:
; CHECK: mcall pc
; CHECK: cp r12, 0
; CHECK-NEXT: srne r12
; CHECK: .long __avr32_f64_cmp_ge
entry:
  %cmp = fcmp ole double %a, %b
  %ret = zext i1 %cmp to i32
  ret i32 %ret
}
