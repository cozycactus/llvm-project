; RUN: llc -mtriple=avr32 -O2 -debug-pass=Structure < %s -o /dev/null 2>&1 | FileCheck %s

; AVR32 keeps pre-RA MachineSink enabled; target-specific shouldSink controls
; the AVR32 instructions that are unsafe to move.
; CHECK: Machine Common Subexpression Elimination
; CHECK: Machine code sinking
; CHECK: Peephole Optimizations
; CHECK: PostRA Machine Sink

define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  ret i32 %sum
}
