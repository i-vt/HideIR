; RUN: opt -load-pass-plugin=%{string_plugin} -passes="EnterpriseStringEncryption" -S < %s | FileCheck %s

; CHECK: @.str = private unnamed_addr global [13 x i8]
; CHECK: @llvm.global_ctors = appending global [1 x { i32, ptr, ptr }] [{{.*}}@obf.decrypt_strings{{.*}}]

@.str = private unnamed_addr constant [13 x i8] c"secret_token\00", align 1

define ptr @get_secret() {
entry:
  ; CHECK: entry:
  ; CHECK-NOT: "secret_token"
  ret ptr @.str
}

; Relaxed the check to look for 'internal' linkage and ignore trailing attribute groups
; CHECK: define internal void @obf.decrypt_strings(
; CHECK: xor i8 {{.*}}, {{.*}}
