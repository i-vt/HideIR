; RUN: opt -load-pass-plugin=%{api_hiding_plugin} -passes="EnterpriseAPIHiding" -S < %s | FileCheck %s

target triple = "x86_64-unknown-linux-gnu"

; External function declaration (like a libc call)
declare i32 @puts(ptr)

@.str = private unnamed_addr constant [6 x i8] c"hello\00"

define void @caller() {
entry:
  ; CHECK: @obf.api.puts = private unnamed_addr constant [5 x i8] c"puts\00"
  ; This direct call should be replaced with an indirect call via dlsym
  ; CHECK-NOT: call i32 @puts(
  ; CHECK: call ptr @dlsym(
  ; CHECK: call i32 %{{.*}}(ptr @.str)
  call i32 @puts(ptr @.str)
  ret void
}

; Verify dlsym is declared
; CHECK: declare ptr @dlsym(ptr, ptr)
