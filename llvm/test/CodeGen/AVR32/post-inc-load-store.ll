; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck %s

define i32 @sum8(ptr %p, i32 %n) minsize optsize {
; CHECK-LABEL: sum8:
; CHECK: ld.ub {{r[0-9]+}}, {{r[0-9]+}}++
entry:
  br label %loop

loop:
  %cur = phi ptr [ %p, %entry ], [ %next, %body ]
  %i = phi i32 [ %n, %entry ], [ %dec, %body ]
  %sum = phi i32 [ 0, %entry ], [ %sum.next, %body ]
  %done = icmp eq i32 %i, 0
  br i1 %done, label %exit, label %body

body:
  %v = load i8, ptr %cur, align 1
  %z = zext i8 %v to i32
  %sum.next = add i32 %sum, %z
  %next = getelementptr i8, ptr %cur, i32 1
  %dec = add i32 %i, -1
  br label %loop

exit:
  ret i32 %sum
}

define i32 @sum16s(ptr %p, i32 %n) minsize optsize {
; CHECK-LABEL: sum16s:
; CHECK: ld.sh {{r[0-9]+}}, {{r[0-9]+}}++
entry:
  br label %loop

loop:
  %cur = phi ptr [ %p, %entry ], [ %next, %body ]
  %i = phi i32 [ %n, %entry ], [ %dec, %body ]
  %sum = phi i32 [ 0, %entry ], [ %sum.next, %body ]
  %done = icmp eq i32 %i, 0
  br i1 %done, label %exit, label %body

body:
  %v = load i16, ptr %cur, align 2
  %s = sext i16 %v to i32
  %sum.next = add i32 %sum, %s
  %next = getelementptr i16, ptr %cur, i32 1
  %dec = add i32 %i, -1
  br label %loop

exit:
  ret i32 %sum
}

define i32 @sum32(ptr %p, i32 %n) minsize optsize {
; CHECK-LABEL: sum32:
; CHECK: ld.w {{r[0-9]+}}, {{r[0-9]+}}++
entry:
  br label %loop

loop:
  %cur = phi ptr [ %p, %entry ], [ %next, %body ]
  %i = phi i32 [ %n, %entry ], [ %dec, %body ]
  %sum = phi i32 [ 0, %entry ], [ %sum.next, %body ]
  %done = icmp eq i32 %i, 0
  br i1 %done, label %exit, label %body

body:
  %v = load i32, ptr %cur, align 4
  %sum.next = add i32 %sum, %v
  %next = getelementptr i32, ptr %cur, i32 1
  %dec = add i32 %i, -1
  br label %loop

exit:
  ret i32 %sum
}

define void @copy8(ptr %dst, ptr %src, i32 %n) minsize optsize {
; CHECK-LABEL: copy8:
; CHECK: ld.ub {{r[0-9]+}}, {{r[0-9]+}}++
; CHECK: st.b {{r[0-9]+}}++, {{r[0-9]+}}
entry:
  br label %loop

loop:
  %d = phi ptr [ %dst, %entry ], [ %d.next, %body ]
  %s = phi ptr [ %src, %entry ], [ %s.next, %body ]
  %i = phi i32 [ %n, %entry ], [ %dec, %body ]
  %done = icmp eq i32 %i, 0
  br i1 %done, label %exit, label %body

body:
  %v = load i8, ptr %s, align 1
  store i8 %v, ptr %d, align 1
  %d.next = getelementptr i8, ptr %d, i32 1
  %s.next = getelementptr i8, ptr %s, i32 1
  %dec = add i32 %i, -1
  br label %loop

exit:
  ret void
}
