#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
CONFIG_DIR="$SCRIPT_DIR/orchestrator/config"
WRAPPER="$BUILD_DIR/compiler_wrapper"
DEFAULT_CONFIG="$CONFIG_DIR/default_config.yaml"

VERBOSE=false
SOURCE_FILES=()
COMPILER_ARGS=()
OUTPUT=""
SKIP_TESTS=false

usage() {
    cat << EOF
Usage: $(basename "$0") [options] <source files> [-- <extra compiler flags>]

Compiles C/C++ files with HideIR obfuscation.

Options:
  -o <file>      Output binary name (default: a.out)
  -v, --verbose  Show full build and test output
  -s, --skip-tests  Skip pre-flight tests
  -c <file>      Use a custom config YAML
  -h, --help     Show this help

Examples:
  $(basename "$0") main.c -o myapp
  $(basename "$0") main.c utils.c -o myapp -- -lm -lpthread
  $(basename "$0") -v main.cpp -o app
EOF
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)    usage ;;
        -v|--verbose) VERBOSE=true; shift ;;
        -s|--skip-tests) SKIP_TESTS=true; shift ;;
        -c)           CUSTOM_CONFIG="$2"; shift 2 ;;
        -o)           OUTPUT="$2"; shift 2 ;;
        --)           shift; COMPILER_ARGS+=("$@"); break ;;
        -*)           COMPILER_ARGS+=("$1"); shift ;;
        *)            SOURCE_FILES+=("$1"); shift ;;
    esac
done

if [[ ${#SOURCE_FILES[@]} -eq 0 ]]; then
    echo "Error: no source files specified." >&2
    usage
fi

if [[ -z "$OUTPUT" ]]; then
    OUTPUT="a.out"
fi

export OBFUSCATOR_CONFIG="${CUSTOM_CONFIG:-$DEFAULT_CONFIG}"

# --- Build passes if needed ---
build_passes() {
    if [[ ! -x "$WRAPPER" ]]; then
        echo "[hideir] Building passes (first run)..."
        (cd "$SCRIPT_DIR" && chmod +x build.sh && ./build.sh) 2>&1 \
            | if $VERBOSE; then cat; else cat > /dev/null; fi
        echo "[hideir] Build complete."
    fi
}

# --- Run pre-flight tests ---
run_tests() {
    if $SKIP_TESTS; then return 0; fi

    local test_src
    test_src=$(mktemp /tmp/hideir_test_XXXX.c)
    cat > "$test_src" << 'TESTEOF'
#include <stdio.h>
const char *msg = "HIDEIR_PREFLIGHT_TOKEN";
int main() { printf("%s\n", msg); return 0; }
TESTEOF

    local test_bin="${test_src%.c}"
    local ok=true

    # Compile test
    if ! "$WRAPPER" "$test_src" -o "$test_bin" -O1 2>&1 \
        | if $VERBOSE; then cat; else cat > /dev/null; fi; then
        echo "[hideir] ERROR: test compilation failed." >&2
        rm -f "$test_src" "$test_bin"
        return 1
    fi

    # Run test
    local out
    out=$("$test_bin" 2>/dev/null) || true
    if echo "$out" | grep -q "HIDEIR_PREFLIGHT_TOKEN"; then
        if $VERBOSE; then echo "[hideir] Pre-flight: runtime output correct."; fi
    else
        echo "[hideir] ERROR: obfuscated test binary produced wrong output." >&2
        ok=false
    fi

    # String encryption check
    if strings "$test_bin" | grep -q "HIDEIR_PREFLIGHT_TOKEN"; then
        echo "[hideir] WARNING: string encryption may not be working." >&2
    elif $VERBOSE; then
        echo "[hideir] Pre-flight: string encryption verified."
    fi

    # Symbol stripping check
    if nm "$test_bin" 2>&1 | grep -q "no symbols"; then
        if $VERBOSE; then echo "[hideir] Pre-flight: symbols stripped."; fi
    else
        if $VERBOSE; then echo "[hideir] Pre-flight: symbols not stripped (strip_symbols may be off)."; fi
    fi

    rm -f "$test_src" "$test_bin"
    $ok
}

# --- Main ---
build_passes
run_tests

# Determine compiler (clang vs clang++) from file extensions
USE_CXX=false
for f in "${SOURCE_FILES[@]}"; do
    case "$f" in
        *.cpp|*.cc|*.cxx|*.C|*.CPP) USE_CXX=true ;;
    esac
done

# Build the command
CMD=("$WRAPPER" "${SOURCE_FILES[@]}" -o "$OUTPUT" -O1 "${COMPILER_ARGS[@]}")

if $VERBOSE; then
    echo "[hideir] ${CMD[*]}"
fi

"${CMD[@]}"

echo "[hideir] $OUTPUT"
