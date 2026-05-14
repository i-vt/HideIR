; RUN: opt -load-pass-plugin=%{split_plugin} -passes="EnterpriseSplitBasicBlock" -S < %s | FileCheck %s

; A large block with many arithmetic instructions should be split
define i32 @big_computation(i32 %a, i32 %b, i32 %c) {
entry:
  %v1 = add i32 %a, %b
  %v2 = mul i32 %v1, %c
  %v3 = sub i32 %v2, %a
  %v4 = add i32 %v3, %b
  %v5 = mul i32 %v4, 2
  %v6 = add i32 %v5, %c
  %v7 = sub i32 %v6, %v1
  %v8 = mul i32 %v7, %v2
  %v9 = add i32 %v8, 42
  %v10 = xor i32 %v9, %v3
  ret i32 %v10
}

; The original entry block should be split into at least two blocks
; CHECK: entry:
; CHECK: br label %entry.split
; CHECK: entry.split:
; CHECK: ret i32
