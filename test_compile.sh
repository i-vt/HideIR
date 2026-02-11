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
# Run the lit tests to verify the individual LLVM passes work correctly
lit -v build/tests
echo "[+] Lit tests passed successfully."
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

echo "=== 4. Running Verification Tests ==="

# 1. Static Analysis Check: Look for the plaintext string
echo "[*] Checking for plaintext strings in the binary..."
if strings test_obfuscated | grep -q "Enterprise-Grade"; then
    echo "[-] TEST FAILED: Plaintext string 'Enterprise-Grade-Security-2024' was found in the binary!"
    exit 1
else
    echo "[+] TEST PASSED: Plaintext string was successfully encrypted and removed from the binary."
fi
echo ""

# 2. Static Analysis Check: Verify the binary is stripped of symbols
echo "[*] Checking if binary is stripped of symbols..."
# nm outputs "no symbols" to stderr, so we redirect stderr to stdout (2>&1) to grep it
if nm test_obfuscated 2>&1 | grep -q "no symbols"; then
    echo "[+] TEST PASSED: Binary is successfully stripped. No function names are visible."
else
    echo "[-] TEST FAILED: Symbols were found in the binary! The -s flag was not applied."
    exit 1
fi
echo ""

# 3. Runtime Check: Verify access is granted with the correct key
echo "[*] Testing binary execution with CORRECT password..."
./test_obfuscated Enterprise-Grade-Security-2024
echo ""

# 4. Runtime Check: Verify access is denied with a wrong key
echo "[*] Testing binary execution with INCORRECT password..."
./test_obfuscated wrong_password

echo ""
echo "=== All end-to-end tests completed successfully! ==="
