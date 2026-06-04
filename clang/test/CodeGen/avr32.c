// RUN: %clang_cc1 -triple avr32 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avr32 -O1 -S -o - %s | FileCheck --check-prefix=ASM %s
// RUN: %clang_cc1 -triple avr32 -O1 -emit-obj -o %t.o %s
// RUN: not %clang_cc1 -triple avr32 -emit-obj -o /dev/null %s 2>&1 | FileCheck --check-prefix=ERR %s

int add(int a, int b) {
  return a + b;
}

// CHECK: target datalayout = "E-m:e-p:32:32-i64:32-n32-S32"
// CHECK: target triple = "avr32"
// CHECK: define {{.*}}i32 @add(i32 {{.*}}, i32 {{.*}})
// CHECK: add nsw i32

// ASM-LABEL: add:
// ASM: addal r12, r11, r12
// ASM: ret r12

// ERR: fatal error: error in backend: Cannot select: {{.*}}load
