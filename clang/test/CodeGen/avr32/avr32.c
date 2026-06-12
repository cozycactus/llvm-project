// RUN: %clang_cc1 -triple avr32 -fno-signed-char -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avr32 -fno-signed-char -Os -emit-llvm -o - %s | FileCheck --check-prefix=OS %s
// RUN: %clang_cc1 -triple avr32 -fno-signed-char -Oz -emit-llvm -o - %s | FileCheck --check-prefix=OZ %s
// RUN: %clang_cc1 -triple avr32 -fno-signed-char -O1 -S -o - %s | FileCheck --check-prefix=ASM %s
// RUN: %clang_cc1 -triple avr32 -fno-signed-char -O1 -emit-obj -o %t.o %s
// RUN: %clang_cc1 -triple avr32 -fno-signed-char -S -o - %s | FileCheck --check-prefix=O0ASM %s
// RUN: %clang_cc1 -triple avr32 -fno-signed-char -emit-obj -o %t.o %s
// RUN: llvm-readobj -r %t.o | FileCheck --check-prefix=RELOC %s

int add(int a, int b) {
  return a + b;
}

// OS: define{{.*}} i32 @add{{.*}} #[[OS_ATTRS:[0-9]+]]
// OS: attributes #[[OS_ATTRS]] = { {{.*}}"function-inline-threshold"="5"{{.*}} }
// OZ: define{{.*}} i32 @add{{.*}} #[[OZ_ATTRS:[0-9]+]]
// OZ-NOT: attributes #[[OZ_ATTRS]] = { {{.*}}"function-inline-threshold"

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

unsigned get_dr(void) {
  return __builtin_mfdr(0);
}

void set_dr(unsigned value) {
  __builtin_mtdr(0, value);
}

void write_tlb(void) {
  __builtin_tlbw();
}

void read_tlb(void) {
  __builtin_tlbr();
}

void search_tlb(void) {
  __builtin_tlbs();
}

unsigned short swap_16(unsigned short value) {
  return __builtin_bswap_16(value);
}

unsigned swap_32(unsigned value) {
  return __builtin_bswap_32(value);
}

int eq(int a, int b) {
  return a == b;
}

int char_to_int(char value) {
  return value;
}

int char_is_negative(void) {
  return (char)0xff < 0;
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

int load_two_bytes(void) {
  return bytes[2] + bytes[3];
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

unsigned mask_byte(unsigned x) {
  return x & 255u;
}

unsigned mask_half(unsigned x) {
  return x & 65535u;
}

unsigned bitfield_extract(unsigned x) {
  return (x >> 13) & 7;
}

unsigned bitfield_insert_low(unsigned old, unsigned value) {
  return (old & ~63u) | (value & 63u);
}

unsigned bitfield_insert_mid(unsigned old, unsigned value) {
  return (old & ~(0xffffu << 8)) | ((value & 0xffffu) << 8);
}

unsigned bitfield_insert_const(unsigned old) {
  return (old & 0x00ffffffu) | 0xa5000000u;
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

#define AVR32_BRANCH_GLOBAL(N) int branch_global_##N
#define AVR32_BRANCH_STORE(N) branch_sink = branch_global_##N

volatile int branch_sink;
AVR32_BRANCH_GLOBAL(0);
AVR32_BRANCH_GLOBAL(1);
AVR32_BRANCH_GLOBAL(2);
AVR32_BRANCH_GLOBAL(3);
AVR32_BRANCH_GLOBAL(4);
AVR32_BRANCH_GLOBAL(5);
AVR32_BRANCH_GLOBAL(6);
AVR32_BRANCH_GLOBAL(7);
AVR32_BRANCH_GLOBAL(8);
AVR32_BRANCH_GLOBAL(9);
AVR32_BRANCH_GLOBAL(10);
AVR32_BRANCH_GLOBAL(11);
AVR32_BRANCH_GLOBAL(12);
AVR32_BRANCH_GLOBAL(13);
AVR32_BRANCH_GLOBAL(14);
AVR32_BRANCH_GLOBAL(15);
AVR32_BRANCH_GLOBAL(16);
AVR32_BRANCH_GLOBAL(17);
AVR32_BRANCH_GLOBAL(18);
AVR32_BRANCH_GLOBAL(19);
AVR32_BRANCH_GLOBAL(20);
AVR32_BRANCH_GLOBAL(21);
AVR32_BRANCH_GLOBAL(22);
AVR32_BRANCH_GLOBAL(23);
AVR32_BRANCH_GLOBAL(24);
AVR32_BRANCH_GLOBAL(25);
AVR32_BRANCH_GLOBAL(26);
AVR32_BRANCH_GLOBAL(27);
AVR32_BRANCH_GLOBAL(28);
AVR32_BRANCH_GLOBAL(29);
AVR32_BRANCH_GLOBAL(30);
AVR32_BRANCH_GLOBAL(31);
AVR32_BRANCH_GLOBAL(32);
AVR32_BRANCH_GLOBAL(33);
AVR32_BRANCH_GLOBAL(34);
AVR32_BRANCH_GLOBAL(35);
AVR32_BRANCH_GLOBAL(36);
AVR32_BRANCH_GLOBAL(37);
AVR32_BRANCH_GLOBAL(38);
AVR32_BRANCH_GLOBAL(39);

void branch_over_lda_w_size(int x) {
  if (x == 0)
    return;
  AVR32_BRANCH_STORE(0);
  AVR32_BRANCH_STORE(1);
  AVR32_BRANCH_STORE(2);
  AVR32_BRANCH_STORE(3);
  AVR32_BRANCH_STORE(4);
  AVR32_BRANCH_STORE(5);
  AVR32_BRANCH_STORE(6);
  AVR32_BRANCH_STORE(7);
  AVR32_BRANCH_STORE(8);
  AVR32_BRANCH_STORE(9);
  AVR32_BRANCH_STORE(10);
  AVR32_BRANCH_STORE(11);
  AVR32_BRANCH_STORE(12);
  AVR32_BRANCH_STORE(13);
  AVR32_BRANCH_STORE(14);
  AVR32_BRANCH_STORE(15);
  AVR32_BRANCH_STORE(16);
  AVR32_BRANCH_STORE(17);
  AVR32_BRANCH_STORE(18);
  AVR32_BRANCH_STORE(19);
  AVR32_BRANCH_STORE(20);
  AVR32_BRANCH_STORE(21);
  AVR32_BRANCH_STORE(22);
  AVR32_BRANCH_STORE(23);
  AVR32_BRANCH_STORE(24);
  AVR32_BRANCH_STORE(25);
  AVR32_BRANCH_STORE(26);
  AVR32_BRANCH_STORE(27);
  AVR32_BRANCH_STORE(28);
  AVR32_BRANCH_STORE(29);
  AVR32_BRANCH_STORE(30);
  AVR32_BRANCH_STORE(31);
  AVR32_BRANCH_STORE(32);
  AVR32_BRANCH_STORE(33);
  AVR32_BRANCH_STORE(34);
  AVR32_BRANCH_STORE(35);
  AVR32_BRANCH_STORE(36);
  AVR32_BRANCH_STORE(37);
  AVR32_BRANCH_STORE(38);
  AVR32_BRANCH_STORE(39);
}

#undef AVR32_BRANCH_GLOBAL
#undef AVR32_BRANCH_STORE

__attribute__((optnone, noinline)) int pick(int a, int b) {
  if (a == b)
    return 1;
  return 0;
}

// CHECK: target datalayout = "E-m:e-p:32:32-i64:32-f64:32-n32-S32"
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
// CHECK: define {{.*}}i32 @get_dr()
// CHECK: call i32 asm sideeffect "mfdr $0, $1"
// CHECK: define {{.*}}void @set_dr(i32 {{.*}})
// CHECK: call void asm sideeffect "mtdr $0, $1"
// CHECK: define {{.*}}void @write_tlb()
// CHECK: call void asm sideeffect "tlbw"
// CHECK: define {{.*}}void @read_tlb()
// CHECK: call void asm sideeffect "tlbr"
// CHECK: define {{.*}}void @search_tlb()
// CHECK: call void asm sideeffect "tlbs"
// CHECK: define {{.*}}zeroext i16 @swap_16(i16 {{.*}})
// CHECK: call i16 @llvm.bswap.i16
// CHECK: define {{.*}}i32 @eq(i32 {{.*}}, i32 {{.*}})
// CHECK: icmp eq i32
// CHECK: define {{.*}}i32 @char_to_int(i8 {{.*}}zeroext
// CHECK: zext i8
// CHECK: define {{.*}}i32 @char_is_negative()
// CHECK: ret i32 0
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
// CHECK: define {{.*}}i32 @bitfield_insert_low(i32 {{.*}}, i32 {{.*}})
// CHECK: and i32
// CHECK: or i32
// CHECK: define {{.*}}i32 @bitfield_insert_mid(i32 {{.*}}, i32 {{.*}})
// CHECK: shl i32
// CHECK: or i32
// CHECK: define {{.*}}i32 @bitfield_insert_const(i32 {{.*}})
// CHECK: and i32
// CHECK: or i32
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
// CHECK: define {{.*}}void @branch_over_lda_w_size(i32 {{.*}})
// CHECK: load i32, ptr @branch_global_0
// CHECK: store volatile i32
// CHECK: define {{.*}}i32 @pick(i32 {{.*}}, i32 {{.*}})
// CHECK: icmp eq i32

// ASM-LABEL: add:
// ASM: add r12, r11
// ASM: ret r12

// ASM-LABEL: call_indirect:
// ASM: mov {{r[0-9]+}}, r12
// ASM: mov r12, r11
// ASM: icall {{r[0-9]+}}
// ASM: popm r4-r7, pc

// ASM-LABEL: call_indirect_u8:
// ASM: mov {{r[0-9]+}}, r12
// ASM: mov r12, r11
// ASM: icall {{r[0-9]+}}
// ASM: popm r4-r7, pc

// ASM-LABEL: get_sr:
// ASM: mfsr r12, 0
// ASM: ret r12

// ASM-LABEL: set_sr:
// ASM: mtsr 0, r12
// ASM: ret r12

// ASM-LABEL: get_dr:
// ASM: mfdr r12, 0
// ASM: ret r12

// ASM-LABEL: set_dr:
// ASM: mtdr 0, r12
// ASM: ret r12

// ASM-LABEL: write_tlb:
// ASM: tlbw
// ASM: ret r12

// ASM-LABEL: read_tlb:
// ASM: tlbr
// ASM: ret r12

// ASM-LABEL: search_tlb:
// ASM: tlbs
// ASM: ret r12

// ASM-LABEL: swap_16:
// ASM: swap.b r12
// ASM: lsr r12, 16
// ASM: ret r12

// ASM-LABEL: eq:
// ASM: cp r12, r11
// ASM: sreq r12
// ASM: ret r12

// ASM-LABEL: char_to_int:
// ASM-NOT: casts.b
// ASM: ret r12

// ASM-LABEL: char_is_negative:
// ASM: mov r12, 0
// ASM: ret r12

// ASM-LABEL: addr:
// ASM: lddpc [[BYTES_ADDR:r[0-9]+]], pc[.Ltmp
// ASM: ret [[BYTES_ADDR]]
// ASM: .long bytes

// ASM-LABEL: load_byte:
// ASM: lddpc [[BYTE_BASE:r[0-9]+]], pc[.Ltmp
// ASM: ld.ub r12, [[BYTE_BASE]][3]
// ASM: ret r12
// ASM: .long bytes

// ASM-LABEL: load_two_bytes:
// ASM: lddpc [[BYTES_BASE:r[0-9]+]], pc[.Ltmp
// ASM: ld.ub [[BYTE0:r[0-9]+]], [[BYTES_BASE]][2]
// ASM: ld.ub [[BYTE1:r[0-9]+]], [[BYTES_BASE]][3]
// ASM: add{{.*}} r12, [[BYTE1]]
// ASM: ret r12
// ASM: .long bytes

// ASM-LABEL: load_table:
// ASM: and
// ASM: lddpc [[VALUES_BASE:r[0-9]+]], pc[.Ltmp
// ASM: ld.w r12, [[VALUES_BASE]][{{r[0-9]+}} << 2]
// ASM: ret r12
// ASM: .long values

// ASM-LABEL: or_mask:
// ASM: mov [[MASK:r[0-9]+]], 65536
// ASM: or r12, [[MASK]]
// ASM: ret r12

// ASM-LABEL: clear_low_bit:
// ASM: mov [[MASK:r[0-9]+]], -2
// ASM: and r12, [[MASK]]
// ASM: ret r12

// ASM-LABEL: xor_mask:
// ASM: eorh r12, 1
// ASM: ret r12

// ASM-LABEL: mask_byte:
// ASM: castu.b r12
// ASM: ret r12

// ASM-LABEL: mask_half:
// ASM: castu.h r12
// ASM: ret r12

// ASM-LABEL: bitfield_extract:
// ASM: bfextu r12, r12, 13, 3
// ASM: ret r12

// ASM-LABEL: bitfield_insert_low:
// ASM: bfins r12, r11, 0, 6
// ASM: ret r12

// ASM-LABEL: bitfield_insert_mid:
// ASM: bfins r12, r11, 8, 16
// ASM: ret r12

// ASM-LABEL: bitfield_insert_const:
// ASM: mov {{r[0-9]+}}, 165
// ASM: bfins r12, {{r[0-9]+}}, 24, 8
// ASM: ret r12

// ASM-LABEL: count_leading:
// ASM: clz r12, r12
// ASM: ret r12

// ASM-LABEL: count_trailing:
// ASM: brev r12
// ASM: clz r12, r12
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
// ASM: pushm r4-r7
// ASM: popm r4-r7
// ASM: ret r12

// ASM-LABEL: shift_right64:
// ASM: pushm r4-r7
// ASM: popm r4-r7
// ASM: ret r12

// ASM-LABEL: shift_arith64:
// ASM: movcs
// ASM: movcs
// ASM: moveq
// ASM: ret r12

// ASM-LABEL: sign_byte:
// ASM: casts.b r12
// ASM: ret r12

// ASM-LABEL: sign_half:
// ASM: casts.h r12
// ASM: ret r12

// ASM-LABEL: arith_ops:
// ASM-DAG: mul
// ASM-DAG: divs
// ASM: sub
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
// ASM: mcall pc
// ASM: popm r4-r7, pc
// ASM: .long __avr32_udiv64

// ASM-LABEL: double_sub:
// ASM: mcall pc
// ASM: popm r4-r7, pc
// ASM: .long __avr32_f64_sub

// ASM-LABEL: double_div:
// ASM: mcall pc
// ASM: popm r4-r7, pc
// ASM: .long __avr32_f64_div

// ASM-LABEL: double_to_uint:
// ASM: mcall pc
// ASM: popm r4-r7, pc
// ASM: .long __avr32_f64_to_u32

// ASM-LABEL: double_lt:
// ASM: mcall pc
// ASM: srne r12
// ASM: popm r4-r7, pc
// ASM: .long __avr32_f64_cmp_lt

// ASM-LABEL: log10_double:
// ASM: mcall pc
// ASM: popm r4-r7, pc
// ASM: .long log10

// ASM-LABEL: choose_lt:
// ASM: cp r12, 7
// ASM: movcs
// ASM: ret r12

// ASM-LABEL: choose_nonzero:
// ASM: cp r12, 0
// ASM: moveq
// ASM: ret r12

// ASM-LABEL: clear_n:
// ASM: mcall pc
// ASM: popm r4-r7, pc
// ASM: .long memset

// ASM-LABEL: six_arg_callee:
// ASM: lddsp {{r[0-9]+}}, sp[0]
// ASM: ret r12

// ASM-LABEL: call_six_args:
// ASM: sub sp, 4
// ASM: stdsp sp[0], r12
// ASM: mcall pc
// ASM: sub sp, -4
// ASM: popm r4-r7, pc
// ASM: .long six_arg_extern

// ASM-LABEL: call_variadic:
// ASM: sub sp, 20
// ASM: stdsp sp[16], r12
// ASM: mcall pc
// ASM: sub sp, -20
// ASM: popm r4-r7, pc
// ASM: .long variadic_extern

// ASM-LABEL: call_return_u64:
// ASM: mcall pc
// ASM: popm r4-r7, pc
// ASM: .long return_u64

// ASM-LABEL: load_ready:
// ASM: ld.ub r12
// ASM: ret r12

// ASM-LABEL: store_ready:
// ASM: st.b
// ASM: ret r12

// ASM-LABEL: pass_vla:
// ASM: mov r7, sp
// ASM: sub {{r[0-9]+}}, sp
// ASM: mov sp, {{r[0-9]+}}
// ASM: mcall pc
// ASM: mov sp, r7
// ASM: popm r4-r7, pc
// ASM: .long use_ptr

// ASM-LABEL: mmio_address:
// ASM: mov r12, 68
// ASM: sbr r12, 30
// ASM: ret r12

// ASM-LABEL: store_global:
// ASM: lddpc [[SINK_BASE:r[0-9]+]], pc[.Ltmp
// ASM: st.w [[SINK_BASE]][0], r12
// ASM: ret r12

// ASM-LABEL: branch_over_lda_w_size:
// ASM: cp r12, 0
// ASM: breq .LBB
// ASM: lddpc {{r[0-9]+}}, pc[.Ltmp

// O0ASM-LABEL: add:
// O0ASM: sub sp, 8
// O0ASM: stdsp sp[4], r12
// O0ASM: stdsp sp[0], r11
// O0ASM: ld.d {{r[0-9]+}}, sp[0]
// O0ASM: ret r12

// O0ASM-LABEL: main:
// O0ASM: sub sp, 4
// O0ASM: mov r12, 40
// O0ASM: mov r11, 2
// O0ASM: mcall pc
// O0ASM: sub sp, -4
// O0ASM: popm r4-r7, pc
// O0ASM: .long add

// O0ASM-LABEL: pick:
// O0ASM: cp {{r[0-9]+}}, {{r[0-9]+}}
// O0ASM: brne .LBB
// O0ASM: mov {{r[0-9]+}}, 1
// O0ASM: rjmp .LBB
// O0ASM: mov {{r[0-9]+}}, 0
// O0ASM: ret r12

// RELOC: R_AVR32_32_CPENT add 0x0
// RELOC: R_AVR32_32_CPENT bytes 0x0
// RELOC: R_AVR32_32_CPENT bytes 0x0
// RELOC: R_AVR32_32_CPENT bytes 0x0
// RELOC: R_AVR32_32_CPENT values 0x0
// RELOC: R_AVR32_32_CPENT sink 0x0
