package main

import (
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

	"gopkg.in/yaml.v3"
)

// ─── Config types (mirrors interceptor.Config) ───────────────────────────────

type PassConfig struct {
	Global struct {
		Enabled      bool   `yaml:"enabled"       json:"enabled"`
		PluginDir    string `yaml:"plugin_dir"    json:"plugin_dir,omitempty"`
		StripSymbols bool   `yaml:"strip_symbols" json:"strip_symbols"`
	} `yaml:"global" json:"global"`
	Passes struct {
		SplitBasicBlock struct {
			Enabled   bool `yaml:"enabled"    json:"enabled"`
			Threshold int  `yaml:"threshold"  json:"threshold,omitempty"`
		} `yaml:"split_basic_block" json:"split_basic_block"`
		Flattening struct {
			Enabled     bool    `yaml:"enabled"     json:"enabled"`
			Probability float64 `yaml:"probability" json:"probability,omitempty"`
		} `yaml:"flattening" json:"flattening"`
		OpaquePredicate struct {
			Enabled     bool    `yaml:"enabled"     json:"enabled"`
			Probability float64 `yaml:"probability" json:"probability,omitempty"`
		} `yaml:"opaque_predicate" json:"opaque_predicate"`
		StringEncryption struct {
			Enabled bool `yaml:"enabled" json:"enabled"`
		} `yaml:"string_encryption" json:"string_encryption"`
		FunctionOutlining struct {
			Enabled bool `yaml:"enabled" json:"enabled"`
		} `yaml:"function_outlining" json:"function_outlining"`
		AntiDebugging struct {
			Enabled bool `yaml:"enabled" json:"enabled"`
		} `yaml:"anti_debugging" json:"anti_debugging"`
		APIHiding struct {
			Enabled bool `yaml:"enabled" json:"enabled"`
		} `yaml:"api_hiding" json:"api_hiding"`
		AntiTampering struct {
			Enabled bool `yaml:"enabled" json:"enabled"`
		} `yaml:"anti_tampering" json:"anti_tampering"`
	} `yaml:"passes" json:"passes"`
}

// ─── Request / Response types ─────────────────────────────────────────────────

type ObfuscateRequest struct {
	Source       string      `json:"source,omitempty"`
	SourceBase64 string      `json:"source_base64,omitempty"`
	Lang         string      `json:"lang,omitempty"`
	Config       *PassConfig `json:"config,omitempty"`
}

type ObfuscateResponse struct {
	BinaryBase64 string `json:"binary_base64"`
	Log          string `json:"log"`
	DurationMs   int64  `json:"duration_ms"`
}

type ErrorResponse struct {
	Error string `json:"error"`
	Log   string `json:"log,omitempty"`
}

type HealthResponse struct {
	Status    string `json:"status"`
	Timestamp string `json:"timestamp"`
	Version   string `json:"version"`
}

type ConfigResponse struct {
	DefaultConfig PassConfig `json:"default_config"`
}

// ─── Globals ──────────────────────────────────────────────────────────────────

var (
	wrapperPath    string
	defaultCfgPath string
)

// ─── Helpers ──────────────────────────────────────────────────────────────────

func jsonError(w http.ResponseWriter, status int, msg string, logOutput string) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(ErrorResponse{Error: msg, Log: logOutput})
}

func loadDefaultConfig() (*PassConfig, error) {
	data, err := os.ReadFile(defaultCfgPath)
	if err != nil {
		return nil, err
	}
	var cfg PassConfig
	if err := yaml.Unmarshal(data, &cfg); err != nil {
		return nil, err
	}
	return &cfg, nil
}

// ─── Handlers ─────────────────────────────────────────────────────────────────

func handleHealth(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(HealthResponse{
		Status:    "ok",
		Timestamp: time.Now().UTC().Format(time.RFC3339),
		Version:   "1.0.0",
	})
}

func handleGetConfig(w http.ResponseWriter, r *http.Request) {
	cfg, err := loadDefaultConfig()
	if err != nil {
		jsonError(w, 500, "failed to load default config: "+err.Error(), "")
		return
	}
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(ConfigResponse{DefaultConfig: *cfg})
}

func handleObfuscate(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		jsonError(w, 405, "method not allowed; use POST", "")
		return
	}

	start := time.Now()

	var req ObfuscateRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		jsonError(w, 400, "invalid JSON body: "+err.Error(), "")
		return
	}

	// Resolve source
	var srcBytes []byte
	if req.Source != "" {
		srcBytes = []byte(req.Source)
	} else if req.SourceBase64 != "" {
		var err error
		srcBytes, err = base64.StdEncoding.DecodeString(req.SourceBase64)
		if err != nil {
			jsonError(w, 400, "invalid base64 in source_base64: "+err.Error(), "")
			return
		}
	} else {
		jsonError(w, 400, "provide either 'source' or 'source_base64'", "")
		return
	}

	lang := strings.ToLower(req.Lang)
	if lang == "" {
		lang = "c"
	}
	if lang != "c" && lang != "cpp" {
		jsonError(w, 400, "lang must be 'c' or 'cpp'", "")
		return
	}

	// Temp workspace
	workDir, err := os.MkdirTemp("", "hideir-*")
	if err != nil {
		jsonError(w, 500, "failed to create temp dir: "+err.Error(), "")
		return
	}
	defer os.RemoveAll(workDir)

	// Write source file
	srcPath := filepath.Join(workDir, "input."+lang)
	if err := os.WriteFile(srcPath, srcBytes, 0644); err != nil {
		jsonError(w, 500, "failed to write source: "+err.Error(), "")
		return
	}

	// Write config (custom or default)
	cfgPath := defaultCfgPath
	if req.Config != nil {
		cfgBytes, err := yaml.Marshal(req.Config)
		if err != nil {
			jsonError(w, 400, "failed to serialise config: "+err.Error(), "")
			return
		}
		cfgPath = filepath.Join(workDir, "config.yaml")
		if err := os.WriteFile(cfgPath, cfgBytes, 0644); err != nil {
			jsonError(w, 500, "failed to write config: "+err.Error(), "")
			return
		}
	}

	outPath := filepath.Join(workDir, "output")

	cmd := exec.Command(wrapperPath,
		"--skip-tests",
		"-c", cfgPath,
		srcPath,
		"-o", outPath,
	)
	cmd.Env = append(os.Environ(), "OBFUSCATOR_CONFIG="+cfgPath)

	var logBuf strings.Builder
	cmd.Stdout = io.MultiWriter(os.Stdout, &logBuf)
	cmd.Stderr = io.MultiWriter(os.Stderr, &logBuf)

	if err := cmd.Run(); err != nil {
		log.Printf("compilation error: %v\nlog:\n%s", err, logBuf.String())
		jsonError(w, 422, "compilation failed: "+err.Error(), logBuf.String())
		return
	}

	binBytes, err := os.ReadFile(outPath)
	if err != nil {
		jsonError(w, 500, "binary not found after compilation: "+err.Error(), logBuf.String())
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(ObfuscateResponse{
		BinaryBase64: base64.StdEncoding.EncodeToString(binBytes),
		Log:          logBuf.String(),
		DurationMs:   time.Since(start).Milliseconds(),
	})
}

// methodRouter dispatches to the right handler based on HTTP method,
// returning 405 JSON for wrong methods — compatible with Go 1.21 ServeMux.
func methodRouter(handlers map[string]http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		h, ok := handlers[r.Method]
		if !ok {
			allowed := make([]string, 0, len(handlers))
			for m := range handlers {
				allowed = append(allowed, m)
			}
			w.Header().Set("Allow", strings.Join(allowed, ", "))
			jsonError(w, 405, "method not allowed", "")
			return
		}
		h(w, r)
	}
}

// ─── Main ─────────────────────────────────────────────────────────────────────

func main() {
	wrapperPath = os.Getenv("HIDEIR_WRAPPER")
	if wrapperPath == "" {
		wrapperPath = "/hideir/hideir.sh"
	}
	defaultCfgPath = os.Getenv("HIDEIR_DEFAULT_CONFIG")
	if defaultCfgPath == "" {
		defaultCfgPath = "/hideir/orchestrator/config/default_config.yaml"
	}
	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}

	mux := http.NewServeMux()
	mux.HandleFunc("/health",    methodRouter(map[string]http.HandlerFunc{"GET": handleHealth}))
	mux.HandleFunc("/config",    methodRouter(map[string]http.HandlerFunc{"GET": handleGetConfig}))
	mux.HandleFunc("/obfuscate", methodRouter(map[string]http.HandlerFunc{"POST": handleObfuscate}))

	addr := fmt.Sprintf(":%s", port)
	log.Printf("HideIR API listening on %s  (wrapper=%s  config=%s)", addr, wrapperPath, defaultCfgPath)
	log.Fatal(http.ListenAndServe(addr, mux))
}
