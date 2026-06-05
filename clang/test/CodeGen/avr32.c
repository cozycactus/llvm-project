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

unsigned short swap_16(unsigned short value) {
  return __builtin_bswap_16(value);
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

unsigned clear_low_bit(unsigned x) {
  return x & ~1u;
}

unsigned xor_mask(unsigned x) {
  return x ^ 65536u;
}

unsigned bitfield_extract(unsigned x) {
  return (x >> 13) & 7;
}

int count_leading(unsigned x) {
  return __builtin_clz(x);
}

int count_trailing(unsigned x) {
  return __builtin_ctz(x);
}

int count_population(unsigned x) {
  return __builtin_popcount(x);
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

unsigned long long shift_left64(unsigned long long x, unsigned y) {
  return x << (y & 63);
}

unsigned long long shift_right64(unsigned long long x, unsigned y) {
  return x >> (y & 63);
}

long long shift_arith64(long long x, unsigned y) {
  return x >> (y & 63);
}

int sign_byte(unsigned x) {
  return (signed char)x;
}

int sign_half(unsigned x) {
  return (short)x;
}

int arith_ops(int a, int b) {
  return (a * b) - (a / b);
}

unsigned unsigned_div(unsigned a, unsigned b) {
  return a / b;
}

int signed_rem(int a, int b) {
  return a % b;
}

unsigned unsigned_rem(unsigned a, unsigned b) {
  return a % b;
}

unsigned long long unsigned_div64(unsigned long long a, unsigned long long b) {
  return a / b;
}

double double_sub(double a, double b) {
  return a - b;
}

double double_div(double a, double b) {
  return a / b;
}

unsigned double_to_uint(double x) {
  return (unsigned)x;
}

int double_lt(double a, double b) {
  return a < b;
}

double log10_double(double x) {
  return __builtin_log10(x);
}

int choose_lt(unsigned c, int a, int b) {
  return c < 7 ? a : b;
}

int choose_nonzero(int c, int a, int b) {
  return c ? a : b;
}

void clear_n(char *p, unsigned n) {
  __builtin_memset(p, 0, n);
}

int six_arg_callee(int a, int b, int c, int d, int e, int f) {
  return a + f;
}

extern int six_arg_extern(int, int, int, int, int, int);
int call_six_args(int x) {
  return six_arg_extern(1, 2, 3, 4, 5, x);
}

extern int variadic_extern(const char *, ...);
int call_variadic(int x) {
  return variadic_extern("%d", 1, 2, 3, 4, x);
}

extern unsigned long long return_u64(void);
unsigned call_return_u64(void) {
  return (unsigned)return_u64();
}

_Bool ready;
int load_ready(void) {
  return ready;
}

void store_ready(_Bool value) {
  ready = value;
}

int dense_switch(int x) {
  switch (x) {
  case 0:
    return 11;
  case 1:
    return 13;
  case 2:
    return 17;
  case 3:
    return 19;
  case 4:
    return 23;
  case 5:
    return 29;
  case 6:
    return 31;
  case 7:
    return 37;
  default:
    return 0;
  }
}

extern int use_ptr(int *);
int pass_local(int value) {
  int local = value;
  return use_ptr(&local);
}

int pass_vla(unsigned n) {
  int local[n];
  local[0] = n;
  return use_ptr(local);
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
// CHECK: define {{.*}}zeroext i16 @swap_16(i16 {{.*}})
// CHECK: call i16 @llvm.bswap.i16
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
// CHECK: define {{.*}}i32 @clear_low_bit(i32 {{.*}})
// CHECK: and i32
// CHECK: define {{.*}}i32 @xor_mask(i32 {{.*}})
// CHECK: xor i32
// CHECK: define {{.*}}i32 @bitfield_extract(i32 {{.*}})
// CHECK: lshr i32
// CHECK: and i32
// CHECK: define {{.*}}i32 @count_leading(i32 {{.*}})
// CHECK: call i32 @llvm.ctlz.i32
// CHECK: define {{.*}}i32 @count_trailing(i32 {{.*}})
// CHECK: call i32 @llvm.cttz.i32
// CHECK: define {{.*}}i32 @count_population(i32 {{.*}})
// CHECK: call i32 @llvm.ctpop.i32
// CHECK: define {{.*}}i32 @shift_left(i32 {{.*}}, i32 {{.*}})
// CHECK: shl i32
// CHECK: define {{.*}}i32 @shift_right(i32 {{.*}}, i32 {{.*}})
// CHECK: lshr i32
// CHECK: define {{.*}}i32 @shift_arith(i32 {{.*}}, i32 {{.*}})
// CHECK: ashr i32
// CHECK: define {{.*}}i64 @shift_left64(i64 {{.*}}, i32 {{.*}})
// CHECK: shl i64
// CHECK: define {{.*}}i64 @shift_right64(i64 {{.*}}, i32 {{.*}})
// CHECK: lshr i64
// CHECK: define {{.*}}i64 @shift_arith64(i64 {{.*}}, i32 {{.*}})
// CHECK: ashr i64
// CHECK: define {{.*}}i32 @sign_byte(i32 {{.*}})
// CHECK: sext i8
// CHECK: define {{.*}}i32 @sign_half(i32 {{.*}})
// CHECK: sext i16
// CHECK: define {{.*}}i32 @arith_ops(i32 {{.*}}, i32 {{.*}})
// CHECK: mul nsw i32
// CHECK: sdiv i32
// CHECK: sub nsw i32
// CHECK: define {{.*}}i32 @unsigned_div(i32 {{.*}}, i32 {{.*}})
// CHECK: udiv i32
// CHECK: define {{.*}}i32 @signed_rem(i32 {{.*}}, i32 {{.*}})
// CHECK: srem i32
// CHECK: define {{.*}}i32 @unsigned_rem(i32 {{.*}}, i32 {{.*}})
// CHECK: urem i32
// CHECK: define {{.*}}i64 @unsigned_div64(i64 {{.*}}, i64 {{.*}})
// CHECK: udiv i64
// CHECK: define {{.*}}double @double_sub(double {{.*}}, double {{.*}})
// CHECK: fsub double
// CHECK: define {{.*}}double @double_div(double {{.*}}, double {{.*}})
// CHECK: fdiv double
// CHECK: define {{.*}}i32 @double_to_uint(double {{.*}})
// CHECK: fptoui double
// CHECK: define {{.*}}i32 @double_lt(double {{.*}}, double {{.*}})
// CHECK: fcmp olt double
// CHECK: define {{.*}}double @log10_double(double {{.*}})
// CHECK: call double @llvm.log10.f64
// CHECK: define {{.*}}i32 @choose_lt(i32 {{.*}}, i32 {{.*}}, i32 {{.*}})
// CHECK: icmp ult i32
// CHECK: define {{.*}}i32 @choose_nonzero(i32 {{.*}}, i32 {{.*}}, i32 {{.*}})
// CHECK: icmp ne i32
// CHECK: define {{.*}}void @clear_n(ptr {{.*}}, i32 {{.*}})
// CHECK: call void @llvm.memset
// CHECK: define {{.*}}i32 @six_arg_callee
// CHECK: define {{.*}}i32 @call_six_args
// CHECK: call i32 @six_arg_extern
// CHECK: define {{.*}}i32 @call_variadic
// CHECK: call i32 (ptr, ...) @variadic_extern
// CHECK: define {{.*}}i32 @call_return_u64()
// CHECK: call i64 @return_u64
// CHECK: define {{.*}}i32 @load_ready()
// CHECK: load i8, ptr @ready
// CHECK: define {{.*}}void @store_ready(i1
// CHECK: store i8
// CHECK: define {{.*}}i32 @dense_switch(i32 {{.*}})
// CHECK: switch i32
// CHECK: define {{.*}}i32 @pass_local(i32 {{.*}})
// CHECK: call i32 @use_ptr
// CHECK: define {{.*}}i32 @pass_vla(i32 {{.*}})
// CHECK: alloca i32, i32
// CHECK: call i32 @use_ptr
// CHECK: define {{.*}}i32 @mmio_address()
// CHECK: ret i32 1073741892
// CHECK: define {{.*}}void @store_global(i32 {{.*}})
// CHECK: store i32
// CHECK: define {{.*}}i32 @pick(i32 {{.*}}, i32 {{.*}})
// CHECK: icmp eq i32

// ASM-LABEL: add:
// ASM: add r12, r11
// ASM: ret r12

// ASM-LABEL: call_indirect:
// ASM: mov {{r[0-9]+}}, r12
// ASM: mov r12, r11
// ASM: icall {{r[0-9]+}}
// ASM: popm pc

// ASM-LABEL: call_indirect_u8:
// ASM: mov {{r[0-9]+}}, r12
// ASM: mov r12, r11
// ASM: icall {{r[0-9]+}}
// ASM: popm pc

// ASM-LABEL: get_sr:
// ASM: mfsr r12, 0
// ASM: ret r12

// ASM-LABEL: set_sr:
// ASM: mtsr 0, r12
// ASM: ret r12

// ASM-LABEL: swap_16:
// ASM: lsr
// ASM: lsl
// ASM: or
// ASM: andal
// ASM: ret r12

// ASM-LABEL: eq:
// ASM: cp r12, r11
// ASM: mov {{r[0-9]+}}, 1
// ASM: br{{eq|ne}} .LBB
// ASM: mov {{r[0-9]+}}, 0
// ASM: andal r12
// ASM: ret r12

// ASM-LABEL: addr:
// ASM: mov [[BYTES_ADDR:r[0-9]+]], LO(bytes)
// ASM: orh [[BYTES_ADDR]], HI(bytes)
// ASM: ret r12

// ASM-LABEL: load_byte:
// ASM: mov [[BYTE_BASE:r[0-9]+]], LO(bytes)
// ASM: orh [[BYTE_BASE]], HI(bytes)
// ASM: mov {{r[0-9]+}}, 3
// ASM: ld.ub r12, {{r[0-9]+}}[{{r[0-9]+}} << 0]
// ASM: ret r12

// ASM-LABEL: load_table:
// ASM: and
// ASM: mov [[VALUES_BASE:r[0-9]+]], LO(values)
// ASM: orh [[VALUES_BASE]], HI(values)
// ASM: ld.w r12, {{r[0-9]+}}[{{r[0-9]+}} << 2]
// ASM: ret r12

// ASM-LABEL: or_mask:
// ASM: sbr r12, 16
// ASM: ret r12

// ASM-LABEL: clear_low_bit:
// ASM: cbr r12, 0
// ASM: ret r12

// ASM-LABEL: xor_mask:
// ASM: mov {{r[0-9]+}}, 65536
// ASM: eor
// ASM: ret r12

// ASM-LABEL: bitfield_extract:
// ASM: bfextu r12, r12, 13, 3
// ASM: ret r12

// ASM-LABEL: count_leading:
// ASM: pushm r0-r3
// ASM: popm r0-r3, pc

// ASM-LABEL: count_trailing:
// ASM: ret r12

// ASM-LABEL: count_population:
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

// ASM-LABEL: shift_left64:
// ASM: pushm r0-r3
// ASM: popm r0-r3, pc

// ASM-LABEL: shift_right64:
// ASM: pushm r0-r3
// ASM: popm r0-r3, pc

// ASM-LABEL: shift_arith64:
// ASM: ret r12

// ASM-LABEL: sign_byte:
// ASM: lsl
// ASM: asr
// ASM: ret r12

// ASM-LABEL: sign_half:
// ASM: lsl
// ASM: asr
// ASM: ret r12

// ASM-LABEL: arith_ops:
// ASM-DAG: mul
// ASM-DAG: divs
// ASM: subal
// ASM: ret r12

// ASM-LABEL: unsigned_div:
// ASM: divu
// ASM: ret r12

// ASM-LABEL: signed_rem:
// ASM: divs
// ASM: mul
// ASM: sub
// ASM: ret r12

// ASM-LABEL: unsigned_rem:
// ASM: divu
// ASM: mul
// ASM: sub
// ASM: ret r12

// ASM-LABEL: unsigned_div64:
// ASM: rcall __udivdi3
// ASM: popm pc

// ASM-LABEL: double_sub:
// ASM: rcall __subdf3
// ASM: popm pc

// ASM-LABEL: double_div:
// ASM: rcall __divdf3
// ASM: popm pc

// ASM-LABEL: double_to_uint:
// ASM: rcall __fixunsdfsi
// ASM: popm pc

// ASM-LABEL: double_lt:
// ASM: rcall __ltdf2
// ASM: popm pc

// ASM-LABEL: log10_double:
// ASM: rcall log10
// ASM: popm pc

// ASM-LABEL: choose_lt:
// ASM: mov {{r[0-9]+}}, 7
// ASM: cp r12, {{r[0-9]+}}
// ASM: brcs
// ASM: ret r12

// ASM-LABEL: choose_nonzero:
// ASM: cp r12
// ASM: br{{eq|ne}}
// ASM: ret r12

// ASM-LABEL: clear_n:
// ASM: rcall memset
// ASM: popm pc

// ASM-LABEL: six_arg_callee:
// ASM: ld.w {{r[0-9]+}}, sp[0]
// ASM: ret r12

// ASM-LABEL: call_six_args:
// ASM: sub sp, 4
// ASM: st.w sp[0], r12
// ASM: rcall six_arg_extern
// ASM: sub sp, -4
// ASM: popm pc

// ASM-LABEL: call_variadic:
// ASM: sub sp, 4
// ASM: st.w sp[0], r12
// ASM: rcall variadic_extern
// ASM: sub sp, -4
// ASM: popm pc

// ASM-LABEL: call_return_u64:
// ASM: rcall return_u64
// ASM: popm pc

// ASM-LABEL: load_ready:
// ASM: ld.ub r12
// ASM: ret r12

// ASM-LABEL: store_ready:
// ASM: st.b
// ASM: ret r12

// ASM-LABEL: pass_vla:
// ASM: mov r7, sp
// ASM: subal {{r[0-9]+}}, sp
// ASM: mov sp, {{r[0-9]+}}
// ASM: rcall use_ptr
// ASM: mov sp, r7
// ASM: popm r4-r7, pc

// ASM-LABEL: mmio_address:
// ASM: mov r12, 68
// ASM: sbr r12, 30
// ASM: ret r12

// ASM-LABEL: store_global:
// ASM: mov [[SINK_BASE:r[0-9]+]], LO(sink)
// ASM: orh [[SINK_BASE]], HI(sink)
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
// O0ASM: rcall add
// O0ASM: sub sp, -4
// O0ASM: popm pc

// O0ASM-LABEL: pick:
// O0ASM: cp {{r[0-9]+}}, {{r[0-9]+}}
// O0ASM: brne .LBB
// O0ASM: rjmp .LBB
// O0ASM: mov {{r[0-9]+}}, 1
// O0ASM: mov {{r[0-9]+}}, 0
// O0ASM: ret r12

// RELOC: R_AVR32_22H_PCREL add
// RELOC: R_AVR32_LO16 bytes 0x0
// RELOC: R_AVR32_HI16 bytes 0x0
// RELOC: R_AVR32_LO16 bytes 0x0
// RELOC: R_AVR32_HI16 bytes 0x0
// RELOC: R_AVR32_LO16 values 0x0
// RELOC: R_AVR32_HI16 values 0x0
// RELOC: R_AVR32_LO16 sink 0x0
// RELOC: R_AVR32_HI16 sink 0x0
