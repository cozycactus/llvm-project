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

int call_indirect(int (*fn)(int), int x) {
  return fn(x);
}

unsigned char call_indirect_u8(unsigned char (*fn)(unsigned char),
                               unsigned char x) {
  return fn(x);
}

unsigned get_sr(void) {
  return __builtin_mfsr(0);
}

void set_sr(unsigned value) {
  __builtin_mtsr(0, value);
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

unsigned or_mask(unsigned x) {
  return x | 65536u;
}

unsigned xor_mask(unsigned x) {
  return x ^ 65536u;
}

unsigned shift_left(unsigned x, unsigned y) {
  return x << (y & 31);
}

unsigned shift_right(unsigned x, unsigned y) {
  return x >> (y & 31);
}

int shift_arith(int x, unsigned y) {
  return x >> (y & 31);
}

extern int use_ptr(int *);
int pass_local(int value) {
  int local = value;
  return use_ptr(&local);
}

unsigned mmio_address(void) {
  return 0x40000044u;
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
// CHECK: define {{.*}}i32 @call_indirect(ptr {{.*}}, i32 {{.*}})
// CHECK: call i32 %
// CHECK: define {{.*}}zeroext i8 @call_indirect_u8(ptr {{.*}}, i8 {{.*}})
// CHECK: call zeroext i8 %
// CHECK: define {{.*}}i32 @get_sr()
// CHECK: call i32 asm sideeffect "mfsr $0, $1"
// CHECK: define {{.*}}void @set_sr(i32 {{.*}})
// CHECK: call void asm sideeffect "mtsr $0, $1"
// CHECK: define {{.*}}i32 @eq(i32 {{.*}}, i32 {{.*}})
// CHECK: icmp eq i32
// CHECK: define {{.*}}ptr @addr()
// CHECK: ret ptr @bytes
// CHECK: define {{.*}}i32 @load_byte()
// CHECK: load i8, ptr getelementptr
// CHECK: define {{.*}}i32 @load_table(i32 {{.*}})
// CHECK: load i32, ptr
// CHECK: define {{.*}}i32 @or_mask(i32 {{.*}})
// CHECK: or i32
// CHECK: define {{.*}}i32 @xor_mask(i32 {{.*}})
// CHECK: xor i32
// CHECK: define {{.*}}i32 @shift_left(i32 {{.*}}, i32 {{.*}})
// CHECK: shl i32
// CHECK: define {{.*}}i32 @shift_right(i32 {{.*}}, i32 {{.*}})
// CHECK: lshr i32
// CHECK: define {{.*}}i32 @shift_arith(i32 {{.*}}, i32 {{.*}})
// CHECK: ashr i32
// CHECK: define {{.*}}i32 @pass_local(i32 {{.*}})
// CHECK: call i32 @use_ptr
// CHECK: define {{.*}}i32 @mmio_address()
// CHECK: ret i32 1073741892
// CHECK: define {{.*}}void @store_global(i32 {{.*}})
// CHECK: store i32
// CHECK: define {{.*}}i32 @pick(i32 {{.*}}, i32 {{.*}})
// CHECK: icmp eq i32

// ASM-LABEL: add:
// ASM: addal r12, r11, r12
// ASM: ret r12

// ASM-LABEL: call_indirect:
// ASM: mov {{r[0-9]+}}, r12
// ASM: mov r12, r11
// ASM: icall {{r[0-9]+}}
// ASM: ret r12

// ASM-LABEL: call_indirect_u8:
// ASM: mov {{r[0-9]+}}, r12
// ASM: mov r12, r11
// ASM: icall {{r[0-9]+}}
// ASM: ret r12

// ASM-LABEL: get_sr:
// ASM: mfsr r12, 0
// ASM: ret r12

// ASM-LABEL: set_sr:
// ASM: mtsr 0, r12
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

// ASM-LABEL: or_mask:
// ASM: mov {{r[0-9]+}}, 65536
// ASM: oral
// ASM: ret r12

// ASM-LABEL: xor_mask:
// ASM: mov {{r[0-9]+}}, 65536
// ASM: eoral
// ASM: ret r12

// ASM-LABEL: shift_left:
// ASM: lsl
// ASM: ret r12

// ASM-LABEL: shift_right:
// ASM: lsr
// ASM: ret r12

// ASM-LABEL: shift_arith:
// ASM: asr
// ASM: ret r12

// ASM-LABEL: mmio_address:
// ASM-DAG: movh {{r[0-9]+}}, 16384
// ASM-DAG: mov {{r[0-9]+}}, 68
// ASM: oral
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
