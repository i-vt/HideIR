#!/bin/bash
set -e

echo "=== Hello World Obfuscation Test ==="
echo ""

# Rebuild to pick up any source changes
echo "[*] Rebuilding passes..."
chmod +x build.sh && ./build.sh 2>&1 | tail -1
echo ""

export OBFUSCATOR_CONFIG=$(pwd)/orchestrator/config/default_config.yaml

# Compile both versions
echo "[*] Compiling reference binary..."
clang hello.c -o hello_reference -O1
echo "[*] Compiling obfuscated binary..."
./build/compiler_wrapper hello.c -o hello_obfuscated -O1
echo ""

# Run both
echo "=== Runtime Output ==="
echo -n "  Reference:  "
./hello_reference
echo -n "  Obfuscated: "
./hello_obfuscated
echo ""

# Compare output
REF_OUT=$(./hello_reference)
OBF_OUT=$(./hello_obfuscated)
if [ "$REF_OUT" = "$OBF_OUT" ]; then
    echo "[+] Output matches."
else
    echo "[-] Output differs!"
fi
echo ""

# Static analysis comparison
echo "=== Static Analysis ==="

echo "[*] Strings check:"
echo -n "  Reference:  "
strings hello_reference | grep -c "Hello" || echo "0"
echo -n "  Obfuscated: "
strings hello_obfuscated | grep -c "Hello" || echo "0"

echo ""
echo "[*] Symbol table (nm):"
echo -n "  Reference:  "
nm hello_reference 2>/dev/null | grep -cE " T | t " || echo "stripped"
echo -n "  Obfuscated: "
nm hello_obfuscated 2>&1 | grep -q "no symbols" && echo "stripped" || nm hello_obfuscated 2>/dev/null | grep -cE " T | t "

echo ""
echo "[*] Dynamic imports (objdump -T):"
echo "  Reference:"
objdump -T hello_reference 2>/dev/null | grep "UND" | awk '{print "    "$NF}'
echo "  Obfuscated:"
objdump -T hello_obfuscated 2>/dev/null | grep "UND" | awk '{print "    "$NF}'

echo ""
echo "[*] Binary size:"
echo "  Reference:  $(wc -c < hello_reference) bytes"
echo "  Obfuscated: $(wc -c < hello_obfuscated) bytes"

echo ""
echo "[*] Disassembly comparison (main or .text):"
echo "  Reference instruction count:  $(objdump -d hello_reference 2>/dev/null | grep -c '^\s')"
echo "  Obfuscated instruction count: $(objdump -d hello_obfuscated 2>/dev/null | grep -c '^\s')"

echo ""
echo "=== Done ==="
