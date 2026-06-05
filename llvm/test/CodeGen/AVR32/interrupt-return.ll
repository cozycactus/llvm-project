; RUN: llc -mtriple=avr32 < %s | FileCheck %s

@sink = global i32 0

define void @isr_empty() #0 {
; CHECK-LABEL: isr_empty:
; CHECK: rete
  ret void
}

define void @isr_clobber() #0 {
; CHECK-LABEL: isr_clobber:
; CHECK: pushm r8-r9
; CHECK: popm r8-r9
; CHECK: rete
  store volatile i32 42, ptr @sink
  ret void
}

define void @normal() {
; CHECK-LABEL: normal:
; CHECK: ret
; CHECK-NOT: rete
  ret void
}

attributes #0 = { "interrupt" }
