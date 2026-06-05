// RUN: %clang_cc1 -triple avr32 -emit-llvm -o - %s | FileCheck %s

__attribute__((interrupt)) void isr(void) {}
__attribute__((__interrupt__)) void isr_alternate_spelling(void) {}

// CHECK: define{{.*}} void @isr() #[[ATTR:[0-9]+]]
// CHECK: define{{.*}} void @isr_alternate_spelling() #[[ATTR]]
// CHECK: attributes #[[ATTR]] = { {{.*}}"interrupt"{{.*}} }
