#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

echo "=== Building Enterprise Obfuscator ==="

# 1. Build the LLVM C++ Passes
echo "[*] Bootstrapping CMake for LLVM Passes..."
mkdir -p build
cd build

# If LLVM is installed in a custom location (e.g., Homebrew on macOS or custom build), 
# you can pass LLVM_DIR to this script. Otherwise, CMake will try to find it automatically.
# Example: LLVM_DIR=/usr/lib/llvm-16/cmake ./build.sh
if [ -n "$LLVM_DIR" ]; then
    cmake -DLLVM_DIR="$LLVM_DIR" ../
else
    cmake ../
fi

echo "[*] Compiling C++ Passes..."
# Automatically use all available CPU cores for faster compilation
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
cd ..

# 2. Build the Go Orchestrator
echo "[*] Building Go Orchestrator..."
cd orchestrator

# Ensure dependencies are tidy
go mod tidy

# Compile the Go wrapper and output it to the root build directory
go build -o ../build/compiler_wrapper cmd/compiler_wrapper.go
cd ..

echo ""
echo "=== Build Complete ==="
echo "The LLVM pass plugins (.so/.dylib) and the compiler_wrapper binary are located in the 'build/' directory."
echo ""
echo "To use the orchestrator, ensure your configuration file environment variable is set:"
echo "  export OBFUSCATOR_CONFIG=$(pwd)/orchestrator/config/default_config.yaml"
echo ""
echo "Then, use the wrapper in place of your standard compiler:"
echo "  $(pwd)/build/compiler_wrapper main.cpp -o main"
