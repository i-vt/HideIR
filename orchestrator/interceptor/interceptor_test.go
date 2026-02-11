package interceptor

import (
	"os"
	"path/filepath"
	"testing"
)

func TestInterceptStripSymbols(t *testing.T) {
	// Setup a temporary directory for mock config files
	tempDir := t.TempDir()

	// Mock Config 1: Strip Symbols Enabled
	configTruePath := filepath.Join(tempDir, "config_true.yaml")
	os.WriteFile(configTruePath, []byte(`
global:
  enabled: true
  plugin_dir: "/tmp/plugins"
  strip_symbols: true
`), 0644)

	// Mock Config 2: Strip Symbols Disabled
	configFalsePath := filepath.Join(tempDir, "config_false.yaml")
	os.WriteFile(configFalsePath, []byte(`
global:
  enabled: true
  plugin_dir: "/tmp/plugins"
  strip_symbols: false
`), 0644)

	tests := []struct {
		name       string
		args       []string
		configPath string
		wantStrip  bool
	}{
		{
			name:       "Linking phase with strip enabled",
			args:       []string{"gcc", "main.c", "-o", "main"},
			configPath: configTruePath,
			wantStrip:  true,
		},
		{
			name:       "Compile-only phase (-c) with strip enabled",
			args:       []string{"gcc", "-c", "main.c", "-o", "main.o"},
			configPath: configTruePath,
			wantStrip:  false, // Should NOT strip when just compiling objects
		},
		{
			name:       "Preprocess-only phase (-E) with strip enabled",
			args:       []string{"gcc", "-E", "main.c"},
			configPath: configTruePath,
			wantStrip:  false,
		},
		{
			name:       "Linking phase with strip disabled",
			args:       []string{"gcc", "main.c", "-o", "main"},
			configPath: configFalsePath,
			wantStrip:  false,
		},
		{
			name:       "Linking phase with manual -s provided by user",
			args:       []string{"gcc", "main.c", "-s", "-o", "main"},
			configPath: configTruePath,
			wantStrip:  true, // Should have exactly one -s (no duplicates)
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			gotArgs := Intercept(tt.args, tt.configPath)

			// Count occurrences of the "-s" flag in the modified arguments
			stripCount := 0
			for _, arg := range gotArgs {
				if arg == "-s" {
					stripCount++
				}
			}

			if tt.wantStrip && stripCount != 1 {
				t.Errorf("Intercept() expected exactly 1 '-s' flag, got %d. Args: %v", stripCount, gotArgs)
			}
			if !tt.wantStrip && stripCount > 0 {
				t.Errorf("Intercept() expected 0 '-s' flags, got %d. Args: %v", stripCount, gotArgs)
			}
		})
	}
}
