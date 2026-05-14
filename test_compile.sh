#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

echo "=== 1. Building Enterprise Obfuscator ==="
# Ensure the build script is executable and run it to rebuild the plugins and wrapper
chmod +x build.sh
./build.sh
echo "[+] Project build complete."
echo ""

echo "=== 2. Running LLVM Integrated Tests (lit) ==="
# Find lit under its various names (distro packages, pip installs, versioned binaries)
LIT_BIN=""
for candidate in lit llvm-lit llvm-lit-19 llvm-lit-18 llvm-lit-17; do
    if command -v "$candidate" &>/dev/null; then
        LIT_BIN="$candidate"
        break
    fi
done

if [ -n "$LIT_BIN" ]; then
    $LIT_BIN -v build/tests
    echo "[+] Lit tests passed successfully."
else
    echo "[!] 'lit' not found in PATH. Install with: pip install lit"
    echo "[!] Skipping LLVM integrated tests."
fi
echo ""

echo "=== 3. Compiling Obfuscated Binary ==="
# Set the config path for the orchestrator
export OBFUSCATOR_CONFIG=$(pwd)/orchestrator/config/default_config.yaml

# Compile the test file using the Go compiler wrapper
# -O1 is used to ensure standard LLVM optimizations (like GlobalOpt) run,
# verifying that our volatile memory access fix prevents optimization bypass.
./build/compiler_wrapper test_obfuscator.c -o test_obfuscated -O1

echo "[+] Binary compilation successful."
echo ""

# Also compile a clean reference binary for comparison
echo "[*] Compiling reference (non-obfuscated) binary for comparison..."
clang test_obfuscator.c -o test_reference -O1
echo "[+] Reference binary compiled."
echo ""

echo "=== 4. Running Verification Tests ==="
PASS_COUNT=0
FAIL_COUNT=0

pass_test() { echo "[+] TEST PASSED: $1"; PASS_COUNT=$((PASS_COUNT + 1)); }
fail_test() { echo "[-] TEST FAILED: $1"; FAIL_COUNT=$((FAIL_COUNT + 1)); }

# --- Static Analysis Tests ---

# 1. String Encryption: plaintext should not appear in binary
echo "[*] Test 1: Checking for plaintext strings in the binary..."
if strings test_obfuscated | grep -q "Enterprise-Grade"; then
    fail_test "Plaintext string 'Enterprise-Grade-Security-2024' was found in the binary."
else
    pass_test "Plaintext string was successfully encrypted and removed from the binary."
fi

# 2. Symbol Stripping: nm should report no symbols
echo "[*] Test 2: Checking if binary is stripped of symbols..."
if nm test_obfuscated 2>&1 | grep -q "no symbols"; then
    pass_test "Binary is successfully stripped. No function names are visible."
else
    fail_test "Symbols were found in the binary. The -s flag was not applied."
fi

# 3. API Hiding: common libc functions should not appear as direct dynamic imports
echo "[*] Test 3: Checking API hiding (dynamic symbol table)..."
# In the reference binary, printf/strcmp/puts should appear as FUNC UND (imported)
# In the obfuscated binary, they should be resolved via dlsym at runtime instead
HIDDEN_COUNT=0
for func in printf strcmp puts; do
    if objdump -T test_reference 2>/dev/null | grep -qw "$func"; then
        # Function IS a direct import in the reference binary
        if objdump -T test_obfuscated 2>/dev/null | grep -qw "$func"; then
            : # Still visible — not hidden
        else
            HIDDEN_COUNT=$((HIDDEN_COUNT + 1))
        fi
    fi
done
if [ "$HIDDEN_COUNT" -gt 0 ]; then
    pass_test "API hiding removed $HIDDEN_COUNT libc function(s) from the dynamic symbol table."
else
    fail_test "No libc functions were hidden from the dynamic symbol table."
fi

# 4. Anti-debugging: dlsym should be present (used by API hiding for runtime resolution)
echo "[*] Test 4: Checking for dlsym presence (runtime API resolution)..."
if objdump -T test_obfuscated 2>/dev/null | grep -qw "dlsym"; then
    pass_test "dlsym is present in the binary (API hiding active)."
else
    fail_test "dlsym not found. API hiding may not be working."
fi

# 5. Function outlining: obfuscated binary should have no recognizable user function names
echo "[*] Test 5: Checking that user function names are hidden..."
if readelf -s test_obfuscated 2>/dev/null | grep -qE "process_data|main"; then
    fail_test "User function names (process_data/main) are still visible in the symbol table."
else
    pass_test "User function names are not visible in the symbol table."
fi

# --- Runtime Correctness Tests ---

# 6. Correct password should grant access
echo "[*] Test 6: Runtime test with CORRECT password..."
OUTPUT_CORRECT=$(./test_obfuscated Enterprise-Grade-Security-2024 2>&1)
if echo "$OUTPUT_CORRECT" | grep -q "Access Granted"; then
    pass_test "Correct password produces 'Access Granted'."
else
    fail_test "Correct password did not produce 'Access Granted'. Output: $OUTPUT_CORRECT"
fi

# 7. Wrong password should deny access
echo "[*] Test 7: Runtime test with INCORRECT password..."
OUTPUT_WRONG=$(./test_obfuscated wrong_password 2>&1)
if echo "$OUTPUT_WRONG" | grep -q "Access Denied"; then
    pass_test "Wrong password produces 'Access Denied'."
else
    fail_test "Wrong password did not produce 'Access Denied'. Output: $OUTPUT_WRONG"
fi

# 8. No arguments should deny access
echo "[*] Test 8: Runtime test with NO arguments..."
OUTPUT_NONE=$(./test_obfuscated 2>&1)
if echo "$OUTPUT_NONE" | grep -q "Access Denied"; then
    pass_test "No arguments produces 'Access Denied'."
else
    fail_test "No arguments did not produce 'Access Denied'. Output: $OUTPUT_NONE"
fi

# 9. Functional output: process_data result should be deterministic
echo "[*] Test 9: Checking deterministic computation output..."
EXPECTED_LINE="Input: 42, Processed Result:"
if echo "$OUTPUT_CORRECT" | grep -q "$EXPECTED_LINE"; then
    pass_test "process_data output is present and matches expected format."
else
    fail_test "process_data output missing or malformed."
fi

# 10. Obfuscated and reference binaries should produce identical functional output
echo "[*] Test 10: Comparing obfuscated vs reference binary output..."
REF_OUTPUT=$(./test_reference Enterprise-Grade-Security-2024 2>&1)
if [ "$OUTPUT_CORRECT" = "$REF_OUTPUT" ]; then
    pass_test "Obfuscated binary output matches the reference binary exactly."
else
    fail_test "Output differs between obfuscated and reference binaries."
    echo "    Reference: $REF_OUTPUT"
    echo "    Obfuscated: $OUTPUT_CORRECT"
fi

# --- Summary ---
echo ""
echo "=== Test Summary ==="
TOTAL=$((PASS_COUNT + FAIL_COUNT))
echo "  Passed: $PASS_COUNT / $TOTAL"
echo "  Failed: $FAIL_COUNT / $TOTAL"
if [ "$FAIL_COUNT" -gt 0 ]; then
    echo ""
    echo "=== Some tests FAILED ==="
    exit 1
fi
echo ""
echo "=== All end-to-end tests completed successfully! ==="
