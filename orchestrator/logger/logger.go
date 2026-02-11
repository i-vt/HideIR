package logger

import (
	"io"
	"log"
	"os"
	"time"
)

var (
	InfoLogger  *log.Logger
	ErrorLogger *log.Logger
	DebugLogger *log.Logger
)

func init() {
	// Open a log file for enterprise telemetry and build debugging
	logFile, err := os.OpenFile("obfuscator_build.log", os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0666)
	if err != nil {
		logFile = os.Stdout // Fallback to stdout if we can't write to the directory
	}

	InfoLogger = log.New(logFile, "INFO: ", log.Ldate|log.Ltime|log.Lshortfile)
	ErrorLogger = log.New(logFile, "ERROR: ", log.Ldate|log.Ltime|log.Lshortfile)

	// Debug logger only outputs if an environment variable is set
	if os.Getenv("OBFUSCATOR_DEBUG") != "" {
		DebugLogger = log.New(os.Stdout, "DEBUG: ", log.Ldate|log.Ltime|log.Lshortfile)
	} else {
		// Fix: Use io.Discard instead of os.DevNull
		DebugLogger = log.New(io.Discard, "", 0)
	}
}

// TrackTime is used to measure how much overhead the obfuscator is adding to compile times
func TrackTime(start time.Time, name string) {
	elapsed := time.Since(start)
	InfoLogger.Printf("%s completed in %s", name, elapsed)
}
