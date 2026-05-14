; RUN: opt -load-pass-plugin=%{anti_debug_plugin} -passes="EnterpriseAntiDebugging" -S < %s | FileCheck %s

target triple = "x86_64-unknown-linux-gnu"

define i32 @simple_func(i32 %x) {
entry:
  %res = add i32 %x, 1
  ret i32 %res
}

; Verify the anti-debug constructor is created and registered
; CHECK: @llvm.global_ctors = appending global {{.*}}@obf.anti_debug_init{{.*}}

; Verify the constructor calls ptrace (Linux) and traps on detection
; CHECK: define internal void @obf.anti_debug_init()
; CHECK: call i64 @ptrace(
; CHECK: icmp eq i64
; CHECK: br i1
; CHECK: trap:
; CHECK: call void @llvm.trap()
; CHECK: unreachable
