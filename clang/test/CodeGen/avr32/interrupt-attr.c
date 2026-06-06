// RUN: %clang_cc1 -triple avr32 -emit-llvm -o - %s | FileCheck %s

__attribute__((interrupt)) void isr(void) {}
__attribute__((__interrupt__)) void isr_alternate_spelling(void) {}
__attribute__((interrupt("none"))) void isr_none(void) {}
__attribute__((interrupt("half"))) void isr_half(void) {}
__attribute__((interrupt("full"))) void isr_full(void) {}

// CHECK: define{{.*}} void @isr() #[[NONE:[0-9]+]]
// CHECK: define{{.*}} void @isr_alternate_spelling() #[[NONE]]
// CHECK: define{{.*}} void @isr_none() #[[NONE]]
// CHECK: define{{.*}} void @isr_half() #[[HALF:[0-9]+]]
// CHECK: define{{.*}} void @isr_full() #[[FULL:[0-9]+]]
// CHECK: attributes #[[NONE]] = { {{.*}}"interrupt"{{.*}} }
// CHECK: attributes #[[HALF]] = { {{.*}}"interrupt"="half"{{.*}} }
// CHECK: attributes #[[FULL]] = { {{.*}}"interrupt"="full"{{.*}} }
