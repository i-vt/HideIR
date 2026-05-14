; RUN: opt -load-pass-plugin=%{flattening_plugin} -passes="EnterpriseFlattening" -S < %s | FileCheck %s

; Test flattening with nested if-else and multiple exit paths
define i32 @nested_branches(i32 %x, i32 %y) {
entry:
  %c1 = icmp sgt i32 %x, 0
  br i1 %c1, label %positive, label %negative

positive:
  %c2 = icmp sgt i32 %y, 0
  br i1 %c2, label %both_pos, label %mixed

both_pos:
  %r1 = add i32 %x, %y
  ret i32 %r1

mixed:
  %r2 = sub i32 %x, %y
  ret i32 %r2

negative:
  %r3 = mul i32 %x, %y
  ret i32 %r3
}

; Verify the dispatcher structure is created
; CHECK: %state_var = alloca ptr
; CHECK: store ptr blockaddress(@nested_branches,
; CHECK: br label %dispatch_header

; Verify the indirect branch has all original blocks as destinations
; CHECK: indirect_dispatch:
; CHECK: %load_state = load ptr, ptr %state_var
; CHECK: indirectbr ptr %load_state
