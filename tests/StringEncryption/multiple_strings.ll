; RUN: opt -load-pass-plugin=%{string_plugin} -passes="EnterpriseStringEncryption" -S < %s | FileCheck %s

; Multiple strings should all be encrypted and decrypted
@.str1 = private unnamed_addr constant [12 x i8] c"first_token\00", align 1
@.str2 = private unnamed_addr constant [13 x i8] c"second_token\00", align 1
@.str3 = private unnamed_addr constant [12 x i8] c"third_token\00", align 1

define ptr @get_first() {
entry:
  ret ptr @.str1
}

define ptr @get_second() {
entry:
  ret ptr @.str2
}

define ptr @get_third() {
entry:
  ret ptr @.str3
}

; All three plaintext strings should be gone from the IR
; CHECK-NOT: c"first_token\00"
; CHECK-NOT: c"second_token\00"
; CHECK-NOT: c"third_token\00"

; All three should become mutable globals (not constant)
; CHECK-DAG: @.str1 = private unnamed_addr global [12 x i8]
; CHECK-DAG: @.str2 = private unnamed_addr global [13 x i8]
; CHECK-DAG: @.str3 = private unnamed_addr global [12 x i8]

; global_ctors appears with globals (before functions in LLVM IR output)
; CHECK: @llvm.global_ctors = appending global {{.*}}@obf.decrypt_strings{{.*}}

; Single decryption function handles all strings
; CHECK: define internal void @obf.decrypt_strings()
; CHECK: xor i8
