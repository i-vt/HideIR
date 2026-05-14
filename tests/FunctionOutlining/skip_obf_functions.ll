; RUN: opt -load-pass-plugin=%{outlining_plugin} -passes="EnterpriseFunctionOutlining" -S < %s | FileCheck %s

; Normal function: non-entry blocks should be outlined
define i32 @normal_func(i32 %a) {
entry:
  %cond = icmp sgt i32 %a, 0
  br i1 %cond, label %then, label %else

then:
  %r1 = mul i32 %a, 2
  br label %end

else:
  %r2 = add i32 %a, 10
  br label %end

end:
  %res = phi i32 [ %r1, %then ], [ %r2, %else ]
  ret i32 %res
}

; Function with obf. in its name should NOT be outlined
define i32 @helper.obf.internal(i32 %x) {
entry:
  %cond = icmp eq i32 %x, 0
  br i1 %cond, label %zero, label %nonzero

zero:
  ret i32 0

nonzero:
  %r = add i32 %x, 1
  ret i32 %r
}

; Normal function should produce outlined functions
; CHECK: define {{.*}} @normal_func.obf.outlined

; obf. function's blocks should NOT be outlined
; CHECK-NOT: @helper.obf.internal.obf.outlined
