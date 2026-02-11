; RUN: opt -load-pass-plugin=%{flattening_plugin} -passes="EnterpriseFlattening" -S < %s | FileCheck %s

define i32 @simple_loop(i32 %n) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %next, %loop ]
  %next = add i32 %i, 1
  %cond = icmp slt i32 %next, %n
  br i1 %cond, label %loop, label %exit

exit:
  ret i32 %i
}

; CHECK: entry:
; CHECK: %state_var = alloca ptr
; CHECK: store ptr blockaddress(@simple_loop, %entry_logic), ptr %state_var
; CHECK: br label %dispatch_header

; CHECK: loop:
; CHECK: %[[SEL:.*]] = select i1 %{{.*}}, ptr blockaddress(@simple_loop, %loop), ptr blockaddress(@simple_loop, %exit)
; CHECK: store ptr %[[SEL]], ptr %state_var
; CHECK: br label %loop_end

; CHECK: dispatch_header:
; CHECK: br label %indirect_dispatch

; CHECK: indirect_dispatch:
; CHECK: %[[STATE:.*]] = load ptr, ptr %state_var
; CHECK: indirectbr ptr %[[STATE]], [label %loop, label %exit, label %entry_logic]
