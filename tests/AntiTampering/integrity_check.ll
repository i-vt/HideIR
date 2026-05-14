; RUN: opt -load-pass-plugin=%{anti_tamper_plugin} -passes="EnterpriseAntiTampering" -S < %s | FileCheck %s

define i32 @protected_func(i32 %x) {
entry:
  %res = mul i32 %x, 3
  ret i32 %res
}

; Verify per-function expected hash global is created
; CHECK: @obf.expected_hash.protected_func = private global i32 0

; The protected function appears first in output (it was in the module first).
; Verify runtime integrity check is injected at its entry.
; CHECK: define i32 @protected_func(i32 %x)
; CHECK: hash.loop:
; CHECK: load volatile i8
; CHECK: xor i32
; CHECK: mul i32 {{.*}}, 16777619
; CHECK: hash.end:
; CHECK: load volatile i32, ptr @obf.expected_hash.protected_func
; CHECK: icmp eq i32
; CHECK: br i1 %{{.*}}, label %tamper.cont, label %tamper.trap

; CHECK: tamper.trap:
; CHECK: call void @llvm.trap()
; CHECK: unreachable

; Verify the init constructor computes the baseline hash (appears after protected_func)
; CHECK: define internal void @obf.tamper_init()
; CHECK: hash.loop:
; CHECK: phi i32
; CHECK: phi i32
; CHECK: xor i32
; CHECK: mul i32
; CHECK: store volatile i32 {{.*}}, ptr @obf.expected_hash.protected_func
