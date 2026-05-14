#!/bin/bash

echo "=== Diagnosing pass interaction crash ==="
echo ""

PASSES=("split_basic_block" "flattening" "opaque_predicate" "string_encryption" \
        "function_outlining" "anti_debugging" "api_hiding" "anti_tampering")
CONFIG_DIR=$(pwd)/orchestrator/config

gen_config() {
    # Generate config with all passes enabled except the ones listed in args
    local disabled=("$@")
    cat > "$CONFIG_DIR/diag_config.yaml" << EOF
global:
  enabled: true
  plugin_dir: ""
  strip_symbols: false
passes:
  split_basic_block:
    enabled: true
    threshold: 3
  flattening:
    enabled: true
    probability: 1.0
  opaque_predicate:
    enabled: true
    probability: 0.8
  string_encryption:
    enabled: true
  function_outlining:
    enabled: true
  anti_debugging:
    enabled: true
  api_hiding:
    enabled: true
  anti_tampering:
    enabled: true
EOF
    for d in "${disabled[@]}"; do
        sed -i "/^  ${d}:/,/enabled:/{s/enabled: true/enabled: false/}" "$CONFIG_DIR/diag_config.yaml"
    done
}

run_test() {
    local label="$1"
    export OBFUSCATOR_CONFIG="$CONFIG_DIR/diag_config.yaml"
    ./build/compiler_wrapper hello.c -o hello_diag -O1 2>/dev/null
    OUTPUT=$(./hello_diag 2>/dev/null)
    CODE=$?
    rm -f hello_diag
    if [ $CODE -eq 0 ] && echo "$OUTPUT" | grep -q "Hello"; then
        printf "  %-45s  OK\n" "$label"
        return 0
    else
        printf "  %-45s  FAIL (exit=%d)\n" "$label" "$CODE"
        return 1
    fi
}

# Phase 1: All-except-one (which pass, when removed, fixes the crash?)
echo "[Phase 1] All passes except one:"
for skip in "${PASSES[@]}"; do
    gen_config "$skip"
    run_test "all except $skip"
done

# Phase 2: Pipeline-start only vs optimizer-last only
echo ""
echo "[Phase 2] Pass groups:"
gen_config "split_basic_block" "flattening" "opaque_predicate" "function_outlining"
run_test "pipeline-start only (str+dbg+api+tamper)"

gen_config "string_encryption" "anti_debugging" "api_hiding" "anti_tampering"
run_test "optimizer-last only (split+flat+opaque+outline)"

# Phase 3: Pair with api_hiding (since SIGSEGV points to bad indirect call)
echo ""
echo "[Phase 3] api_hiding + one other pass:"
for pair in "${PASSES[@]}"; do
    [ "$pair" = "api_hiding" ] && continue
    gen_config "${PASSES[@]}"  # disable all
    # Re-enable just api_hiding and the pair
    sed -i "/^  api_hiding:/,/enabled:/{s/enabled: false/enabled: true/}" "$CONFIG_DIR/diag_config.yaml"
    sed -i "/^  ${pair}:/,/enabled:/{s/enabled: false/enabled: true/}" "$CONFIG_DIR/diag_config.yaml"
    run_test "api_hiding + $pair"
done

rm -f "$CONFIG_DIR/diag_config.yaml"
echo ""
echo "=== Done ==="
