// RUN: %clang_cc1 -triple avr32 -emit-llvm -o - %s | FileCheck %s

unsigned read_sr_arg(unsigned reg) {
  return __builtin_mfsr(reg);
}

void write_sr_arg(unsigned reg, unsigned value) {
  __builtin_mtsr(reg, value);
}

unsigned read_dr_arg(unsigned reg) {
  return __builtin_mfdr(reg);
}

void write_dr_arg(unsigned reg, unsigned value) {
  __builtin_mtdr(reg, value);
}

// CHECK-LABEL: define {{.*}}i32 @read_sr_arg
// CHECK: call i32 asm sideeffect "mfsr $0, $1"
// CHECK-LABEL: define {{.*}}void @write_sr_arg
// CHECK: call void asm sideeffect "mtsr $0, $1"
// CHECK-LABEL: define {{.*}}i32 @read_dr_arg
// CHECK: call i32 asm sideeffect "mfdr $0, $1"
// CHECK-LABEL: define {{.*}}void @write_dr_arg
// CHECK: call void asm sideeffect "mtdr $0, $1"
