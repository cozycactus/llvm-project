// RUN: %clang_cc1 %s -verify -fsyntax-only -triple avr32
// expected-no-diagnostics

void side_effect(int);

__attribute__((naked)) void naked_c_body(int value) {
  int local = value + 1;
  side_effect(local);
  __asm__ volatile("rete");
}
