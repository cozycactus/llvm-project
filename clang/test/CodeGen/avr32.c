// RUN: %clang_cc1 -triple avr32 -emit-llvm -o - %s | FileCheck %s
// RUN: not %clang_cc1 -triple avr32 -emit-obj -o /dev/null %s 2>&1 | FileCheck --check-prefix=ERR %s

int add(int a, int b) {
  return a + b;
}

// CHECK: target datalayout = "E-m:e-p:32:32-i64:32-n32-S32"
// CHECK: target triple = "avr32"
// CHECK: define {{.*}}i32 @add(i32 {{.*}}, i32 {{.*}})
// CHECK: add nsw i32

// ERR: error: unable to create target: 'target machine is not available for triple "avr32"'
