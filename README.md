# HideIR
 Cross-platform LLVM-based obfuscation framework, integrating directly into the compiler pipeline. 

## Getting Started
```
python3 -m venv venv
source venv/bin/activate
pip install lit
```

### Testing
```
# Compile
./hideir.sh -c examples/my_config.yaml examples/main.c -o secureapp

# Functionality Test
./secureapp           # invalid licence (digits sum odd)
./secureapp 1234      # valid licence (1+2+3+4 = 10, even)

# Compare
clang examples/main.c -o secureapp_ref -O1
strings secureapp_ref | grep -i "hunter2\|password\|admin"   # plaintext
strings secureapp     | grep -i "hunter2\|password\|admin"   # gone
objdump -T secureapp_ref | grep UND    # strcmp, printf visible
objdump -T secureapp     | grep UND    # replaced by dlsym
```

 ## Features

 - **Control Flow Flattening** — Replaces structured control flow with an indirect-branch dispatcher, defeating static CFG recovery in IDA/Ghidra.
 - **String Encryption** — Encrypts string constants at compile time with a rolling multi-byte XOR key and decrypts them at program startup via a global constructor.
 - **Opaque Predicates** — Injects always-true conditional branches backed by a volatile global, adding unreachable junk code paths that confuse disassemblers.
 - **Basic Block Splitting** — Randomly splits large basic blocks to inflate the CFG and complicate pattern matching. Configurable instruction threshold.
 - **Function Outlining** — Extracts basic blocks into separate `noinline` functions, scattering logic across the binary.
 - **API Hiding** — Replaces direct calls to external functions with runtime resolution via `dlsym`/`GetProcAddress`, hiding imported symbols from static analysis.
 - **Anti-Debugging** — Detects attached debuggers via `ptrace`/`IsDebuggerPresent` at startup and injects `rdtsc`-based timing checks (x86/PPC) to detect single-stepping.
 - **Anti-Tampering** — Computes FNV-1a hashes of each function's machine code at startup and verifies integrity at every function entry.
 - **Go Orchestrator** — A drop-in compiler wrapper that reads a YAML config and transparently injects all enabled passes, requiring zero build system changes.

 ## Proof Of Concept
 (non-obfuscated on the left, after obfuscation on the right)
 
 ### Strings
 
 <img width="1299" height="534" alt="image" src="https://github.com/user-attachments/assets/a4aef918-c52b-4f3e-9a91-7207effd69c7" />

### nm (symbols)

<img width="1299" height="534" alt="image" src="https://github.com/user-attachments/assets/e3c49f74-f563-41a8-a698-abc1ead5a4f1" />

### objdump (dynamic symbols)


<img width="1719" height="269" alt="image" src="https://github.com/user-attachments/assets/14a2f5a5-d919-4a61-b6df-fc08efa9b6d3" />

### objdump (control flow)

<img width="766" height="2058" alt="image" src="https://github.com/user-attachments/assets/df381c72-995b-4cf7-9263-4229f59fdbdc" />

