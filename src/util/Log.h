#pragma once

#include <mutex>
#include <sstream>
#include <string>

namespace util {

// Log severity levels.
// Ordered from least to most severe.
enum class LogLevel {
  kInfo,   // General informational messages
  kWarn,   // Warnings that don't prevent operation
  kError,  // Errors that may affect functionality
};

// Thread-safe logging utility.
//
// This is a singleton logger that writes timestamped messages to stdout.
// It's designed to be simple and dependency-free (no external logging libraries).
//
// Features:
// - Thread-safe: uses a mutex to prevent interleaved messages from multiple threads
// - Timestamps: each log line includes date and time
// - Prefix: optional prefix for filtering (e.g., "rtp-capture")
// - Macros: LOG_INFO, LOG_WARN, LOG_ERROR for convenient usage
//
// Example output:
//   2024-01-15 14:30:45 [INFO] rtp-capture: Service running
//   2024-01-15 14:30:47 [WARN] rtp-capture: Packet lost
//
// The logger is used throughout the codebase to track:
//   - Service startup/shutdown
//   - RTP stream state
//   - Decoding errors and warnings
//   - File writing operations
class Log {
 public:
  // Get the singleton instance.
  // The instance is constructed on first call and lives for the program lifetime.
  // Returns: Reference to the global Log instance
  static Log& Instance();

  // Set a prefix prepended to all log messages.
  // Useful for distinguishing between multiple services or filtering logs.
  // Thread-safe: acquires mutex internally.
  // Example: SetPrefix("rtp-capture") → "[INFO] rtp-capture: message"
  //
  // Param: prefix - String to prefix all messages with (empty string to clear)
  void SetPrefix(const std::string& prefix);

  // Write a log message with the specified severity level.
  // This method is thread-safe; multiple threads can log concurrently.
  // Format: "YYYY-MM-DD HH:MM:SS [LEVEL] <prefix>: <message>"
  //
  // Param: level - Severity of the message (kInfo, kWarn, kError)
  // Param: message - The message content
  void Write(LogLevel level, const std::string& message);

 private:
  // Private constructor for singleton pattern.
  // Use Instance() to access the logger.
  Log() = default;

  // Mutex to ensure thread-safe logging.
  // Prevents interleaved output from concurrent Write() calls.
  std::mutex mutex_;

  // Optional prefix for all log messages.
  // Commonly set to service name for log aggregation.
  std::string prefix_;
};

// Convert LogLevel enum to human-readable string.
// Used when formatting log messages.
//
// Param: level - Log level to convert
// Returns: String representation ("INFO", "WARN", "ERROR", or "UNKNOWN")
std::string ToString(LogLevel level);

}  // namespace util

// Convenience macros for logging at specific severity levels.
// These macros capture file/line context and call the Log::Write method.
// Usage throughout the codebase:
//   LOG_INFO("Service started");
//   LOG_WARN("Frame decode failed, skipping");
//   LOG_ERROR("Failed to open RTP stream: " + error_msg);
#define LOG_INFO(msg) ::util::Log::Instance().Write(::util::LogLevel::kInfo, (msg))
#define LOG_WARN(msg) ::util::Log::Instance().Write(::util::LogLevel::kWarn, (msg))
#define LOG_ERROR(msg) ::util::Log::Instance().Write(::util::LogLevel::kError, (msg))
