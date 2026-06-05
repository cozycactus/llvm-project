// RUN: %clang_cc1 %s -verify -fsyntax-only -triple avr32

struct a { int b; };

struct a test __attribute__((interrupt)); // expected-warning {{'interrupt' attribute only applies to functions}}

__attribute__((interrupt(12))) void foo(void) {} // expected-error {{'interrupt' attribute takes no arguments}}

__attribute__((interrupt)) int bad_return(void) { return 0; } // expected-warning {{AVR32 'interrupt' attribute only applies to functions that have a 'void' return type}}

__attribute__((interrupt)) void bad_param(int a) {} // expected-warning {{AVR32 'interrupt' attribute only applies to functions that have no parameters}}

__attribute__((interrupt)) void isr(void) {}

__attribute__((__interrupt__)) void isr_alternate_spelling(void) {}
