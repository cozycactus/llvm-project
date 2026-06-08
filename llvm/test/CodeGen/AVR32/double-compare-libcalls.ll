; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

define i32 @oeq(double %a, double %b) minsize optsize {
; CHECK-LABEL: oeq:
; CHECK-NOT: __eqdf2
; CHECK: rcall __avr32_f64_cmp_eq
; CHECK-NEXT: mov [[EQ:r[0-9]+]], 0
; CHECK-NEXT: movne [[EQ]], 1
entry:
  %cmp = fcmp oeq double %a, %b
  %ret = zext i1 %cmp to i32
  ret i32 %ret
}

define i32 @oge(double %a, double %b) minsize optsize {
; CHECK-LABEL: oge:
; CHECK-NOT: __gedf2
; CHECK: rcall __avr32_f64_cmp_ge
; CHECK-NEXT: mov [[GE:r[0-9]+]], 0
; CHECK-NEXT: movne [[GE]], 1
entry:
  %cmp = fcmp oge double %a, %b
  %ret = zext i1 %cmp to i32
  ret i32 %ret
}

define i32 @olt(double %a, double %b) minsize optsize {
; CHECK-LABEL: olt:
; CHECK-NOT: __ltdf2
; CHECK: rcall __avr32_f64_cmp_lt
; CHECK-NEXT: mov [[LT:r[0-9]+]], 0
; CHECK-NEXT: movne [[LT]], 1
entry:
  %cmp = fcmp olt double %a, %b
  %ret = zext i1 %cmp to i32
  ret i32 %ret
}

define i32 @ogt(double %a, double %b) minsize optsize {
; CHECK-LABEL: ogt:
; CHECK-NOT: __gtdf2
; CHECK: rcall __avr32_f64_cmp_lt
; CHECK-NEXT: mov [[GT:r[0-9]+]], 0
; CHECK-NEXT: movne [[GT]], 1
entry:
  %cmp = fcmp ogt double %a, %b
  %ret = zext i1 %cmp to i32
  ret i32 %ret
}

define i32 @ole(double %a, double %b) minsize optsize {
; CHECK-LABEL: ole:
; CHECK-NOT: __ledf2
; CHECK: rcall __avr32_f64_cmp_ge
; CHECK-NEXT: mov [[LE:r[0-9]+]], 0
; CHECK-NEXT: movne [[LE]], 1
entry:
  %cmp = fcmp ole double %a, %b
  %ret = zext i1 %cmp to i32
  ret i32 %ret
}
