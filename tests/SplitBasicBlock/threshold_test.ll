; RUN: opt -load-pass-plugin=%{split_plugin} -passes="EnterpriseSplitBasicBlock" -S < %s | FileCheck %s

define i32 @test_split(i32 %a, i32 %b) {
entry:
  ; CHECK: entry:
  ; CHECK-NEXT: %add = add nsw i32 %a, %b
  ; CHECK: br label %entry.split
  %add = add nsw i32 %a, %b
  %sub = sub nsw i32 %a, %b
  %mul = mul nsw i32 %a, %b
  %div = sdiv i32 %a, %b
  ret i32 %div
}

; CHECK: entry.split:
; CHECK: %div = sdiv i32 %a, %b
