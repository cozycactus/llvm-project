// RUN: %clang_cc1 -triple avr32 -emit-llvm -o - %s | FileCheck %s

struct U64AfterPtr {
  void *p;
  unsigned long long x;
  unsigned y;
};

int align_long_long(void) { return _Alignof(unsigned long long); }
int align_double(void) { return _Alignof(double); }
int align_long_double(void) { return _Alignof(long double); }
int preferred_align_long_long(void) { return __alignof__(unsigned long long); }
int align_struct(void) { return _Alignof(struct U64AfterPtr); }
int offset_x(void) { return __builtin_offsetof(struct U64AfterPtr, x); }
int offset_y(void) { return __builtin_offsetof(struct U64AfterPtr, y); }
int size_struct(void) { return sizeof(struct U64AfterPtr); }
int biggest_alignment(void) { return __BIGGEST_ALIGNMENT__; }

unsigned local_u64_low(unsigned hi, unsigned lo) {
  unsigned long long x = ((unsigned long long)hi << 32) | lo;
  volatile unsigned long long *p = &x;
  return (unsigned)*p;
}

// CHECK: target datalayout = "E-m:e-p:32:32-i64:32-f64:32-n32-S32"
// CHECK-LABEL: define{{.*}} i32 @align_long_long(
// CHECK: ret i32 4
// CHECK-LABEL: define{{.*}} i32 @align_double(
// CHECK: ret i32 4
// CHECK-LABEL: define{{.*}} i32 @align_long_double(
// CHECK: ret i32 4
// CHECK-LABEL: define{{.*}} i32 @preferred_align_long_long(
// CHECK: ret i32 4
// CHECK-LABEL: define{{.*}} i32 @align_struct(
// CHECK: ret i32 4
// CHECK-LABEL: define{{.*}} i32 @offset_x(
// CHECK: ret i32 4
// CHECK-LABEL: define{{.*}} i32 @offset_y(
// CHECK: ret i32 12
// CHECK-LABEL: define{{.*}} i32 @size_struct(
// CHECK: ret i32 16
// CHECK-LABEL: define{{.*}} i32 @biggest_alignment(
// CHECK: ret i32 4
// CHECK-LABEL: define{{.*}} i32 @local_u64_low(
// CHECK: alloca i64, align 4
