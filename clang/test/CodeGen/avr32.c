// RUN: %clang_cc1 -triple avr32 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avr32 -O1 -S -o - %s | FileCheck --check-prefix=ASM %s
// RUN: %clang_cc1 -triple avr32 -O1 -emit-obj -o %t.o %s
// RUN: %clang_cc1 -triple avr32 -S -o - %s | FileCheck --check-prefix=O0ASM %s
// RUN: %clang_cc1 -triple avr32 -emit-obj -o %t.o %s
// RUN: llvm-readobj -r %t.o | FileCheck --check-prefix=RELOC %s

int add(int a, int b) {
  return a + b;
}

int main(void) {
  return add(40, 2);
}

int eq(int a, int b) {
  return a == b;
}

unsigned char bytes[4];
int values[4];
int sink;

unsigned char *addr(void) {
  return bytes;
}

int load_byte(void) {
  return bytes[3];
}

int load_table(unsigned x) {
  return values[x & 3];
}

void store_global(int v) {
  sink = v;
}

__attribute__((optnone, noinline)) int pick(int a, int b) {
  if (a == b)
    return 1;
  return 0;
}

// CHECK: target datalayout = "E-m:e-p:32:32-i64:32-n32-S32"
// CHECK: target triple = "avr32"
// CHECK: define {{.*}}i32 @add(i32 {{.*}}, i32 {{.*}})
// CHECK: add nsw i32
// CHECK: define {{.*}}i32 @eq(i32 {{.*}}, i32 {{.*}})
// CHECK: icmp eq i32
// CHECK: define {{.*}}ptr @addr()
// CHECK: ret ptr @bytes
// CHECK: define {{.*}}i32 @load_byte()
// CHECK: load i8, ptr getelementptr
// CHECK: define {{.*}}i32 @load_table(i32 {{.*}})
// CHECK: load i32, ptr
// CHECK: define {{.*}}void @store_global(i32 {{.*}})
// CHECK: store i32
// CHECK: define {{.*}}i32 @pick(i32 {{.*}}, i32 {{.*}})
// CHECK: icmp eq i32

// ASM-LABEL: add:
// ASM: addal r12, r11, r12
// ASM: ret r12

// ASM-LABEL: eq:
// ASM: cp r12, r11
// ASM: mov {{r[0-9]+}}, 1
// ASM: breq .LBB
// ASM: mov {{r[0-9]+}}, 0
// ASM: andal r12
// ASM: ret r12

// ASM-LABEL: addr:
// ASM: mov r12, bytes
// ASM: ret r12

// ASM-LABEL: load_byte:
// ASM: mov {{r[0-9]+}}, 3
// ASM: mov {{r[0-9]+}}, bytes
// ASM: ld.ub r12, {{r[0-9]+}}[{{r[0-9]+}} << 0]
// ASM: ret r12

// ASM-LABEL: load_table:
// ASM: andal
// ASM: mov {{r[0-9]+}}, values
// ASM: ld.w r12, {{r[0-9]+}}[{{r[0-9]+}} << 2]
// ASM: ret r12

// ASM-LABEL: store_global:
// ASM: mov {{r[0-9]+}}, sink
// ASM: st.w {{r[0-9]+}}[0], r12
// ASM: ret r12

// O0ASM-LABEL: add:
// O0ASM: sub sp, 8
// O0ASM: st.w sp[4], r12
// O0ASM: st.w sp[0], r11
// O0ASM: ld.w {{r[0-9]+}}, sp[4]
// O0ASM: ld.w {{r[0-9]+}}, sp[0]
// O0ASM: ret r12

// O0ASM-LABEL: main:
// O0ASM: sub sp, 4
// O0ASM: mov r12, 40
// O0ASM: mov r11, 2
// O0ASM: rcall pc[add]
// O0ASM: sub sp, -4
// O0ASM: ret r12

// O0ASM-LABEL: pick:
// O0ASM: cp {{r[0-9]+}}, {{r[0-9]+}}
// O0ASM: brne .LBB
// O0ASM: bral .LBB
// O0ASM: mov {{r[0-9]+}}, 1
// O0ASM: mov {{r[0-9]+}}, 0
// O0ASM: ret r12

// RELOC: R_AVR32_22H_PCREL add
// RELOC: R_AVR32_21S bytes 0x0
// RELOC: R_AVR32_21S bytes 0x0
// RELOC: R_AVR32_21S values 0x0
// RELOC: R_AVR32_21S sink 0x0
