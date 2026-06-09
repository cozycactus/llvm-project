; RUN: llc -mtriple=avr32 -filetype=asm < %s | FileCheck %s

define i32 @load_mem(ptr %p) {
; CHECK-LABEL: load_mem:
; CHECK: #APP
; CHECK-NEXT: ld.w r12, r12[0]
; CHECK-NEXT: #NO_APP
  %v = tail call i32 asm sideeffect "ld.w $0, $1", "=r,*m"(ptr elementtype(i32) %p)
  ret i32 %v
}

define void @store_global(i32 %v) {
; CHECK-LABEL: store_global:
; CHECK: lddpc [[BASE:r[0-9]+]], pc[.Ltmp
; CHECK: #APP
; CHECK-NEXT: st.w [[BASE]][4], r12
; CHECK-NEXT: #NO_APP
; CHECK: .long global
  tail call void asm sideeffect "st.w $0, $1", "*m,r"(ptr elementtype(i32) getelementptr (i8, ptr @global, i32 4), i32 %v)
  ret void
}

@global = external global [8 x i32]
