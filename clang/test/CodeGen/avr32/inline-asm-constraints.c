// RUN: %clang_cc1 -triple avr32 -O2 -emit-llvm -o - %s | FileCheck --check-prefix=IR %s
// RUN: %clang_cc1 -triple avr32 -O2 -S -o - %s | FileCheck --check-prefix=ASM %s

void ks21_const(void) {
// IR-LABEL: define{{.*}} void @ks21_const(
// IR: call void asm sideeffect "# $0", "@4Ks21"(i32 1048575)
// ASM-LABEL: ks21_const:
// ASM: # 1048575
  asm volatile("# %0" :: "Ks21"(1048575));
}

void rks21_const(void) {
// IR-LABEL: define{{.*}} void @rks21_const(
// IR: call void asm sideeffect "# $0", "r@4Ks21"(i32 -1048576)
// ASM-LABEL: rks21_const:
// ASM: # -1048576
  asm volatile("# %0" :: "rKs21"(-1048576));
}

void rks21_reg(int value) {
// IR-LABEL: define{{.*}} void @rks21_reg(
// IR: call void asm sideeffect "# $0", "r@4Ks21"(i32 %{{.*}})
// ASM-LABEL: rks21_reg:
// ASM: # r
  asm volatile("# %0" :: "rKs21"(value));
}

void ks21r_const(void) {
// IR-LABEL: define{{.*}} void @ks21r_const(
// IR: call void asm sideeffect "# $0", "@4Ks21r"(i32 1)
// ASM-LABEL: ks21r_const:
// ASM: # 1
  asm volatile("# %0" :: "Ks21r"(1));
}
