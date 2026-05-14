; RUN: opt -load-pass-plugin=%{anti_debug_plugin} -passes="EnterpriseAntiDebugging" -S < %s | FileCheck %s

target triple = "x86_64-unknown-linux-gnu"

; 30 non-PHI blocks in a chain — each has a 20% chance of getting a timing
; check, so P(at least one) = 1 - 0.8^30 ≈ 99.9%.
define i32 @long_chain(i32 %x) {
b0:
  %v0 = add i32 %x, 1
  br label %b1
b1:
  %v1 = add i32 %v0, 1
  br label %b2
b2:
  %v2 = add i32 %v1, 1
  br label %b3
b3:
  %v3 = add i32 %v2, 1
  br label %b4
b4:
  %v4 = add i32 %v3, 1
  br label %b5
b5:
  %v5 = add i32 %v4, 1
  br label %b6
b6:
  %v6 = add i32 %v5, 1
  br label %b7
b7:
  %v7 = add i32 %v6, 1
  br label %b8
b8:
  %v8 = add i32 %v7, 1
  br label %b9
b9:
  %v9 = add i32 %v8, 1
  br label %b10
b10:
  %v10 = add i32 %v9, 1
  br label %b11
b11:
  %v11 = add i32 %v10, 1
  br label %b12
b12:
  %v12 = add i32 %v11, 1
  br label %b13
b13:
  %v13 = add i32 %v12, 1
  br label %b14
b14:
  %v14 = add i32 %v13, 1
  br label %b15
b15:
  %v15 = add i32 %v14, 1
  br label %b16
b16:
  %v16 = add i32 %v15, 1
  br label %b17
b17:
  %v17 = add i32 %v16, 1
  br label %b18
b18:
  %v18 = add i32 %v17, 1
  br label %b19
b19:
  %v19 = add i32 %v18, 1
  br label %b20
b20:
  %v20 = add i32 %v19, 1
  br label %b21
b21:
  %v21 = add i32 %v20, 1
  br label %b22
b22:
  %v22 = add i32 %v21, 1
  br label %b23
b23:
  %v23 = add i32 %v22, 1
  br label %b24
b24:
  %v24 = add i32 %v23, 1
  br label %b25
b25:
  %v25 = add i32 %v24, 1
  br label %b26
b26:
  %v26 = add i32 %v25, 1
  br label %b27
b27:
  %v27 = add i32 %v26, 1
  br label %b28
b28:
  %v28 = add i32 %v27, 1
  br label %b29
b29:
  %v29 = add i32 %v28, 1
  ret i32 %v29
}

; Verify readcyclecounter is called (not just declared) for timing on x86
; CHECK: call i64 @llvm.readcyclecounter()

; Verify timing trap block exists
; CHECK: time_trap:
; CHECK: call void @llvm.trap()
; CHECK: unreachable
