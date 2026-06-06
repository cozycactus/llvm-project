// RUN: %clang_cc1 -triple avr32 -fsyntax-only -verify %s

__attribute__((interrupt)) void isr_default(void);
__attribute__((interrupt("none"))) void isr_none(void);
__attribute__((interrupt("half"))) void isr_half(void);
__attribute__((interrupt("full"))) void isr_full(void);

__attribute__((interrupt("bad"))) void bad_mode(void); // expected-warning {{'interrupt' attribute argument not supported: bad}}
__attribute__((interrupt(1))) void non_string(void); // expected-error {{expected string literal as argument of 'interrupt' attribute}}
__attribute__((interrupt("half", "full"))) void too_many(void); // expected-error {{'interrupt' attribute takes no more than 1 argument}}

__attribute__((interrupt)) void with_param(int); // expected-warning {{AVR32 'interrupt' attribute only applies to functions that have no parameters}}
__attribute__((interrupt)) int non_void(void); // expected-warning {{AVR32 'interrupt' attribute only applies to functions that have a 'void' return type}}

int not_function __attribute__((interrupt)); // expected-warning {{'interrupt' attribute only applies to functions}}
