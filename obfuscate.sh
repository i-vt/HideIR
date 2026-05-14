#!/bin/bash
#
# Usage:
#   ./obfuscate.sh <source files...> [compiler flags...]
#
# Examples:
#   ./obfuscate.sh main.c -o main
#   ./obfuscate.sh main.c utils.c -O2 -o myapp
#   ./obfuscate.sh server.cpp -lstdc++ -lpthread -o server
#
# Environment:
#   HIDEIR_CONFIG  — path to custom config YAML (optional)
#   HIDEIR_DEBUG   — set to 1 for verbose output

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WRAPPER="$SCRIPT_DIR/build/compiler_wrapper"
DEFAULT_CONFIG="$SCRIPT_DIR/orchestrator/config/default_config.yaml"

# Preflight
if [ $# -eq 0 ]; then
    echo "HideIR — LLVM obfuscating compiler"
    echo ""
    echo "Usage: $0 <source files...> [compiler flags...]"
    echo ""
    echo "Examples:"
    echo "  $0 main.c -o main"
    echo "  $0 main.c utils.c -O2 -o myapp"
    echo "  $0 app.cpp -lstdc++ -o app"
    exit 0
fi

if [ ! -f "$WRAPPER" ]; then
    echo "[*] First run — building HideIR..."
    chmod +x "$SCRIPT_DIR/build.sh"
    (cd "$SCRIPT_DIR" && ./build.sh)
    echo ""
fi

export OBFUSCATOR_CONFIG="${HIDEIR_CONFIG:-$DEFAULT_CONFIG}"
[ "${HIDEIR_DEBUG:-0}" = "1" ] && export OBFUSCATOR_DEBUG=1

exec "$WRAPPER" "$@"
