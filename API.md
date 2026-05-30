# HideIR API

A lightweight REST API that wraps the HideIR LLVM obfuscation framework, letting you submit C/C++ source code and receive a compiled, obfuscated binary in return.

---

## Quick start

```bash
# Build & run
docker compose up --build

# Verify
curl http://localhost:8080/health
```

---

## Endpoints

### `GET /health`

Returns service health.

```json
{ "status": "ok", "timestamp": "2026-05-30T12:00:00Z", "version": "1.0.0" }
```

---

### `GET /config`

Returns the active default obfuscation config as JSON, so callers can inspect and override individual fields.

```json
{
  "default_config": {
    "global": { "enabled": true, "strip_symbols": true },
    "passes": {
      "flattening":        { "enabled": true, "probability": 1.0 },
      "string_encryption": { "enabled": true },
      "api_hiding":        { "enabled": true },
      ...
    }
  }
}
```

---

### `POST /obfuscate`

Compile and obfuscate C or C++ source code.

#### Request body (JSON)

| Field          | Type   | Required | Description |
|----------------|--------|----------|-------------|
| `source`       | string | one of † | Source code as a plain string |
| `source_base64`| string | one of † | Source code Base64-encoded (binary-safe alternative) |
| `lang`         | string | no       | `"c"` (default) or `"cpp"` |
| `config`       | object | no       | Pass configuration (see `/config` for schema). Omit to use server defaults. |

† Provide exactly one of `source` or `source_base64`.

#### Successful response `200`

```json
{
  "binary_base64": "<base64-encoded ELF binary>",
  "log":           "<compiler stdout+stderr>",
  "duration_ms":   1234
}
```

Decode and save the binary:
```bash
echo "$binary_base64" | base64 -d > my_app && chmod +x my_app
```

#### Error response `4xx / 5xx`

```json
{ "error": "human-readable reason", "log": "<optional compiler output>" }
```

---

## Examples

### Minimal — use server defaults

```bash
curl -s http://localhost:8080/obfuscate \
  -H 'Content-Type: application/json' \
  -d '{
    "source": "#include <stdio.h>\nint main(){printf(\"hello\\n\");return 0;}"
  }' | jq -r .binary_base64 | base64 -d > hello && chmod +x hello && ./hello
```

### Custom config — disable anti-tampering, lower flattening probability

```bash
curl -s http://localhost:8080/obfuscate \
  -H 'Content-Type: application/json' \
  -d '{
    "source": "...",
    "lang": "c",
    "config": {
      "global": { "enabled": true, "strip_symbols": true },
      "passes": {
        "flattening":        { "enabled": true, "probability": 0.5 },
        "string_encryption": { "enabled": true },
        "opaque_predicate":  { "enabled": true, "probability": 0.6 },
        "split_basic_block": { "enabled": true, "threshold": 4 },
        "function_outlining":{ "enabled": true },
        "anti_debugging":    { "enabled": true },
        "api_hiding":        { "enabled": true },
        "anti_tampering":    { "enabled": false }
      }
    }
  }'
```

### Base64 source (for files with special characters)

```bash
SRC=$(base64 -w0 examples/main.c)
curl -s http://localhost:8080/obfuscate \
  -H 'Content-Type: application/json' \
  -d "{\"source_base64\": \"$SRC\", \"lang\": \"c\"}" \
  | jq -r .binary_base64 | base64 -d > secureapp && chmod +x secureapp
```

---

## Environment variables

| Variable               | Default                                              | Description |
|------------------------|------------------------------------------------------|-------------|
| `PORT`                 | `8080`                                               | TCP port to listen on |
| `HIDEIR_WRAPPER`       | `/hideir/hideir.sh`                                  | Path to the hideir shell wrapper |
| `HIDEIR_DEFAULT_CONFIG`| `/hideir/orchestrator/config/default_config.yaml`    | Default YAML config used when no `config` is POSTed |

---

## Building without Docker

```bash
# 1. Build LLVM passes + Go orchestrator as usual
./build.sh

# 2. Build the API server
cd api
go mod tidy
go build -o ../build/hideir-api .

# 3. Run
HIDEIR_WRAPPER=$(pwd)/../hideir.sh \
HIDEIR_DEFAULT_CONFIG=$(pwd)/../orchestrator/config/default_config.yaml \
../build/hideir-api
```
