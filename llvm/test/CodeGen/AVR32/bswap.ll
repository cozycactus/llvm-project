; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

declare i32 @llvm.bswap.i32(i32)
declare i16 @llvm.bswap.i16(i16)

define i32 @bswap32(i32 %x) {
; CHECK-LABEL: bswap32:
; CHECK: swap.b r12
; CHECK-NEXT: ret r12
entry:
  %y = call i32 @llvm.bswap.i32(i32 %x)
  ret i32 %y
}

define i32 @load_bswap32(ptr %p) {
; CHECK-LABEL: load_bswap32:
; CHECK: ld.w r12, r12[0]
; CHECK-NEXT: swap.b r12
; CHECK-NEXT: ret r12
entry:
  %x = load i32, ptr %p, align 4
  %y = call i32 @llvm.bswap.i32(i32 %x)
  ret i32 %y
}

define i32 @zext_bswap16(i16 %x) {
; CHECK-LABEL: zext_bswap16:
; CHECK: swap.b r12
; CHECK-NEXT: lsr r12, 16
; CHECK-NEXT: ret r12
entry:
  %y = call i16 @llvm.bswap.i16(i16 %x)
  %z = zext i16 %y to i32
  ret i32 %z
}

define i32 @load_zext_bswap16(ptr %p) {
; CHECK-LABEL: load_zext_bswap16:
; CHECK: ld.uh r12, r12[0]
; CHECK-NEXT: swap.bh r12
; CHECK-NEXT: ret r12
entry:
  %x = load i16, ptr %p, align 2
  %y = call i16 @llvm.bswap.i16(i16 %x)
  %z = zext i16 %y to i32
  ret i32 %z
}
