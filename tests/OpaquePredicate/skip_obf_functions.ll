; RUN: opt -load-pass-plugin=%{opaque_plugin} -passes="EnterpriseOpaquePredicate" -S < %s | FileCheck %s

; Normal function should get opaque predicates
define i32 @user_func(i32 %x) {
entry:
  %res = add i32 %x, 1
  ret i32 %res
}

; Function containing "obf." should be skipped entirely
define void @some.obf.helper() {
entry:
  ret void
}

; CHECK: define i32 @user_func
; CHECK: %op.key = load volatile i32, ptr @obf.opaque_key
; CHECK: br i1 %op.cmp

; The obf. function should remain untouched — no opaque predicate inserted
; CHECK: define void @some.obf.helper()
; CHECK-NEXT: entry:
; CHECK-NEXT: ret void
