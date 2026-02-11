package interceptor

import (
	"enterprise-obfuscator/logger"
	"os"
	"path/filepath"
	"runtime"
	"strings"

	"gopkg.in/yaml.v3"
)

type Config struct {
	Global struct {
		Enabled      bool   `yaml:"enabled"`
		PluginDir    string `yaml:"plugin_dir"`
		StripSymbols bool   `yaml:"strip_symbols"`
	} `yaml:"global"`
	Passes struct {
		SplitBasicBlock struct {
			Enabled bool `yaml:"enabled"`
		} `yaml:"split_basic_block"`
		Flattening struct {
			Enabled bool `yaml:"enabled"`
		} `yaml:"flattening"`
		OpaquePredicate struct {
			Enabled bool `yaml:"enabled"`
		} `yaml:"opaque_predicate"`
		StringEncryption struct {
			Enabled bool `yaml:"enabled"`
		} `yaml:"string_encryption"`
		FunctionOutlining struct {
			Enabled bool `yaml:"enabled"`
		} `yaml:"function_outlining"`
		AntiDebugging struct {
			Enabled bool `yaml:"enabled"`
		} `yaml:"anti_debugging"`
		APIHiding struct {
			Enabled bool `yaml:"enabled"`
		} `yaml:"api_hiding"`
		AntiTampering struct {
			Enabled bool `yaml:"enabled"`
		} `yaml:"anti_tampering"`
	} `yaml:"passes"`
}

func Intercept(originalArgs []string, configPath string) []string {
	if len(originalArgs) < 2 {
		return originalArgs
	}

	var cfg Config
	data, err := os.ReadFile(configPath)
	if err == nil {
		yaml.Unmarshal(data, &cfg)
	} else {
		logger.ErrorLogger.Printf("Failed to load config at %s, bypassing obfuscation", configPath)
		return originalArgs
	}

	if !cfg.Global.Enabled {
		return originalArgs
	}

	isCxx := false
	isCompiling := false
	isLinking := true 

	for _, arg := range originalArgs[1:] {
		if strings.HasSuffix(arg, ".cpp") || strings.HasSuffix(arg, ".cc") {
			isCxx = true
			isCompiling = true
		} else if strings.HasSuffix(arg, ".c") || arg == "-c" {
			isCompiling = true
		}
		
		if arg == "-c" || arg == "-S" || arg == "-E" {
			isLinking = false
		}
	}

	var newArgs []string
	compilerName := filepath.Base(originalArgs[0])
	
	if strings.Contains(compilerName, "g++") || (compilerName == "compiler_wrapper" && isCxx) {
		newArgs = append(newArgs, "clang++")
	} else if strings.Contains(compilerName, "gcc") || compilerName == "compiler_wrapper" {
		newArgs = append(newArgs, "clang")
	} else {
		newArgs = append(newArgs, originalArgs[0])
	}

	newArgs = append(newArgs, originalArgs[1:]...)

	if isCompiling {
		ext := ".so"
		prefix := "lib"
		if runtime.GOOS == "darwin" {
			ext = ".dylib"
		} else if runtime.GOOS == "windows" {
			ext = ".dll"
			prefix = "" 
		}

		injectPlugin := func(pluginName string) {
			fileName := prefix + pluginName + ext
			path := filepath.Join(cfg.Global.PluginDir, fileName)
			
			pluginArg := "-fpass-plugin=" + path
			for _, arg := range newArgs {
				if arg == pluginArg { return }
			}
			newArgs = append(newArgs, pluginArg)
			logger.DebugLogger.Printf("Injected plugin: %s", path)
		}

		if cfg.Passes.SplitBasicBlock.Enabled { injectPlugin("SplitBasicBlockPass") }
		if cfg.Passes.Flattening.Enabled { injectPlugin("FlatteningPass") }
		if cfg.Passes.OpaquePredicate.Enabled { injectPlugin("OpaquePredicatePass") }
		if cfg.Passes.StringEncryption.Enabled { injectPlugin("StringEncryptionPass") }
		if cfg.Passes.FunctionOutlining.Enabled { injectPlugin("FunctionOutliningPass") }
		if cfg.Passes.AntiDebugging.Enabled { injectPlugin("AntiDebuggingPass") }
		if cfg.Passes.APIHiding.Enabled { injectPlugin("APIHidingPass") }
		if cfg.Passes.AntiTampering.Enabled { injectPlugin("AntiTamperingPass") }
	}

	if cfg.Global.StripSymbols && isLinking {
		stripArg := "-s"
		alreadyStripped := false
		for _, arg := range newArgs {
			if arg == stripArg {
				alreadyStripped = true
				break
			}
		}
		if !alreadyStripped {
			newArgs = append(newArgs, stripArg)
			logger.DebugLogger.Printf("Injected flag: %s (Strip Symbols)", stripArg)
		}
	}

	// We must pass -ldl to the linker on Unix so dlsym resolves correctly for API Hiding
	if isLinking && runtime.GOOS != "windows" && cfg.Passes.APIHiding.Enabled {
		newArgs = append(newArgs, "-ldl")
	}

	return newArgs
}
