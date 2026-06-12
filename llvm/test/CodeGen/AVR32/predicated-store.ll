; RUN: llc -mtriple=avr32 -mcpu=uc3a3256 -verify-machineinstrs < %s | FileCheck --check-prefix=UC3 %s
; RUN: llc -mtriple=avr32 -verify-machineinstrs < %s | FileCheck --check-prefix=GENERIC %s

define void @pred_store_word_eq(ptr %p, i32 %a, i32 %b, i32 %v) minsize optsize {
; UC3-LABEL: pred_store_word_eq:
; UC3:      cp
; UC3-NEXT: st.weq {{r[0-9]+}}[0], {{r[0-9]+}}
; UC3:      ret r12
; GENERIC-LABEL: pred_store_word_eq:
; GENERIC:      cp
; GENERIC-NEXT: brne
; GENERIC:      st.w {{r[0-9]+}}[0], {{r[0-9]+}}
; GENERIC:      ret r12
entry:
  %cond = icmp eq i32 %a, %b
  br i1 %cond, label %store, label %done

store:
  store i32 %v, ptr %p, align 4
  br label %done

done:
  ret void
}

define void @pred_store_target_word_eq(ptr %p, i32 %a, i32 %v) minsize optsize {
; UC3-LABEL: pred_store_target_word_eq:
; UC3:      cp
; UC3-NEXT: st.weq {{r[0-9]+}}[0], {{r[0-9]+}}
; UC3:      ret r12
; GENERIC-LABEL: pred_store_target_word_eq:
; GENERIC:      cp
; GENERIC-NEXT: breq
; GENERIC:      ret r12
; GENERIC:      st.w {{r[0-9]+}}[0], {{r[0-9]+}}
; GENERIC:      ret r12
entry:
  %cond = icmp eq i32 %a, 0
  br i1 %cond, label %store, label %done

done:
  ret void

store:
  store i32 %v, ptr %p, align 4
  ret void
}

define void @pred_store_half_ne(ptr %p, i32 %a, i32 %b, i16 %v) minsize optsize {
; UC3-LABEL: pred_store_half_ne:
; UC3:      cp
; UC3-NEXT: st.hne {{r[0-9]+}}[0], {{r[0-9]+}}
; UC3:      ret r12
; GENERIC-LABEL: pred_store_half_ne:
; GENERIC:      cp
; GENERIC-NEXT: breq
; GENERIC:      st.h {{r[0-9]+}}[0], {{r[0-9]+}}
; GENERIC:      ret r12
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
; UC3-LABEL: pred_store_byte_ugt:
; UC3:      cp
; UC3-NEXT: st.bcs {{r[0-9]+}}[0], {{r[0-9]+}}
; UC3:      ret r12
; GENERIC-LABEL: pred_store_byte_ugt:
; GENERIC:      cp
; GENERIC-NEXT: brcc
; GENERIC:      st.b {{r[0-9]+}}[0], {{r[0-9]+}}
; GENERIC:      ret r12
entry:
  %cond = icmp ugt i32 %a, %b
  br i1 %cond, label %store, label %done

store:
  store i8 %v, ptr %p, align 1
  br label %done

done:
  ret void
}
