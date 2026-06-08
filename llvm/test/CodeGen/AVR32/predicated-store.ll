; RUN: llc -mtriple=avr32 -mcpu=uc3a3256 -verify-machineinstrs < %s | FileCheck %s

define void @pred_store_word_eq(ptr %p, i32 %a, i32 %b, i32 %v) minsize optsize {
; CHECK-LABEL: pred_store_word_eq:
; CHECK:      cp
; CHECK-NEXT: st.weq {{r[0-9]+}}[0], {{r[0-9]+}}
; CHECK-NOT:  brne
; CHECK:      ret r12
entry:
  %cond = icmp eq i32 %a, %b
  br i1 %cond, label %store, label %done

store:
  store i32 %v, ptr %p, align 4
  br label %done

done:
  ret void
}

define void @pred_store_half_ne(ptr %p, i32 %a, i32 %b, i16 %v) minsize optsize {
; CHECK-LABEL: pred_store_half_ne:
; CHECK:      cp
; CHECK-NEXT: st.hne {{r[0-9]+}}[0], {{r[0-9]+}}
; CHECK-NOT:  breq
; CHECK:      ret r12
entry:
  %cond = icmp ne i32 %a, %b
  br i1 %cond, label %store, label %done

store:
  store i16 %v, ptr %p, align 2
  br label %done

done:
  ret void
}

define void @pred_store_byte_ugt(ptr %p, i32 %a, i32 %b, i8 %v) minsize optsize {
; CHECK-LABEL: pred_store_byte_ugt:
; CHECK:      cp
; CHECK-NEXT: st.bcs {{r[0-9]+}}[0], {{r[0-9]+}}
; CHECK-NOT:  brcc
; CHECK:      ret r12
entry:
  %cond = icmp ugt i32 %a, %b
  br i1 %cond, label %store, label %done

store:
  store i8 %v, ptr %p, align 1
  br label %done

done:
  ret void
}
