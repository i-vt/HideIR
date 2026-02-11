package main

import (
	"enterprise-obfuscator/interceptor"
	"enterprise-obfuscator/logger"
	"os"
	"os/exec"
	"path/filepath"
	"syscall"
	"time"
)

func main() {
	// Track the overhead of the orchestrator running
	defer logger.TrackTime(time.Now(), "Compiler Wrapper Total Execution")

	originalArgs := os.Args
	logger.DebugLogger.Printf("Original command intercepted: %v", originalArgs)

	// Determine where the config file is located
	// In production, this might come from an environment variable like $OBFUSCATOR_CONFIG
	configPath := os.Getenv("OBFUSCATOR_CONFIG")
	if configPath == "" {
		// Default to a config file in the same directory as the wrapper
		execDir, _ := filepath.Abs(filepath.Dir(os.Args[0]))
		configPath = filepath.Join(execDir, "../config/default_config.yaml")
	}

	// Intercept the arguments and inject LLVM passes
	modifiedArgs := interceptor.Intercept(originalArgs, configPath)
	logger.DebugLogger.Printf("Modified command to execute: %v", modifiedArgs)

	if len(modifiedArgs) < 1 {
		logger.ErrorLogger.Fatal("No arguments provided to compiler wrapper.")
	}

	// Find the absolute path to the actual compiler (e.g., /usr/bin/clang)
	compilerPath, err := exec.LookPath(modifiedArgs[0])
	if err != nil {
		logger.ErrorLogger.Fatalf("Failed to find compiler '%s' in PATH: %v", modifiedArgs[0], err)
	}

	logger.InfoLogger.Printf("Delegating execution to: %s %v", compilerPath, modifiedArgs[1:])

	// Replace the current Go orchestrator process with the actual compiler process
	// This ensures exit codes, stdout, and stderr map seamlessly to the build system (Make/CMake)
	env := os.Environ()
	err = syscall.Exec(compilerPath, modifiedArgs, env)
	if err != nil {
		logger.ErrorLogger.Fatalf("Failed to execute compiler process: %v", err)
	}
}
