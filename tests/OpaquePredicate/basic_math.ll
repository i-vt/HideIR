; RUN: opt -load-pass-plugin=%{opaque_plugin} -passes="EnterpriseOpaquePredicate" -S < %s | FileCheck %s

define i32 @math_func(i32 %x) {
entry:
; CHECK: entry:
; CHECK: %op.key = load volatile i32, ptr @obf.opaque_key
; CHECK: %op.mul = mul i32 %op.key, {{.*}}
; CHECK: %op.add = add i32 %op.mul, {{.*}}
; CHECK: %op.cmp = icmp eq i32 %op.add, {{.*}}
; CHECK: br i1 %op.cmp, label %op.true, label %op.false
  %res = mul i32 %x, 2
  ret i32 %res
}

; CHECK: op.false:
; CHECK: br label %op.true
