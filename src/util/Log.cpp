#include "util/Log.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace util {

// Get the singleton Log instance.
// The instance is created on first call and persists for program lifetime.
// Thread-safe: static local variable initialization is guaranteed by C++11.
//
// Returns: Reference to the global Log instance
Log& Log::Instance() {
  static Log instance;
  return instance;
}

// Set the prefix for all log messages.
// The prefix is prepended after the level and before the message.
// Common usage: SetPrefix("service-name") for log aggregation.
//
// Thread-safe: acquires mutex_ to prevent race conditions with Write().
//
// Param: prefix - String to prepend to all messages (empty to clear)
void Log::SetPrefix(const std::string& prefix) {
  std::lock_guard<std::mutex> lock(mutex_);
  prefix_ = prefix;
}

// Write a log message with timestamp and severity level.
// This is the core logging function that formats and outputs messages.
// It's thread-safe; concurrent calls from multiple threads won't interleave.
//
// Format: "YYYY-MM-DD HH:MM:SS [LEVEL] <prefix>: <message>\n"
//
// Thread-safe: acquires mutex_ for the entire formatting and output.
// Time is captured after acquiring the lock to ensure accuracy.
//
// Param: level - Severity level (kInfo, kWarn, kError)
// Param: message - The message content
void Log::Write(LogLevel level, const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto now = std::chrono::system_clock::now();
  auto now_time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  localtime_r(&now_time, &tm);

  std::ostringstream out;
  out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
      << " [" << ToString(level) << "] ";
  if (!prefix_.empty()) {
    out << prefix_ << ": ";
  }
  out << message;
  std::cout << out.str() << std::endl;
}

// Convert LogLevel enum to human-readable string.
// Used when formatting log messages.
//
// Param: level - Log level to convert
// Returns: String representation ("INFO", "WARN", "ERROR", or "UNKNOWN")
std::string ToString(LogLevel level) {
  switch (level) {
    case LogLevel::kInfo:
      return "INFO";
    case LogLevel::kWarn:
      return "WARN";
    case LogLevel::kError:
      return "ERROR";
  }
  return "UNKNOWN";
}

}  // namespace util
