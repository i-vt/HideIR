; RUN: opt -load-pass-plugin=%{outlining_plugin} -passes="EnterpriseFunctionOutlining" -S < %s | FileCheck %s

define i32 @target_function(i32 %a, i32 %b) {
entry:
  %cond = icmp eq i32 %a, 0
  br i1 %cond, label %true_block, label %false_block

true_block:
  %add = add i32 %a, %b
  br label %end

false_block:
  %sub = sub i32 %a, %b
  br label %end

end:
  %res = phi i32 [ %add, %true_block ], [ %sub, %false_block ]
  ret i32 %res
}

; CHECK: define {{.*}} @target_function.obf.outlined
