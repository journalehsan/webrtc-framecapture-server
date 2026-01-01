#pragma once

#include <mutex>
#include <sstream>
#include <string>

namespace util {

enum class LogLevel {
  kInfo,
  kWarn,
  kError,
};

class Log {
 public:
  static Log& Instance();

  void SetPrefix(const std::string& prefix);
  void Write(LogLevel level, const std::string& message);

 private:
  Log() = default;

  std::mutex mutex_;
  std::string prefix_;
};

std::string ToString(LogLevel level);

}  // namespace util

#define LOG_INFO(msg) ::util::Log::Instance().Write(::util::LogLevel::kInfo, (msg))
#define LOG_WARN(msg) ::util::Log::Instance().Write(::util::LogLevel::kWarn, (msg))
#define LOG_ERROR(msg) ::util::Log::Instance().Write(::util::LogLevel::kError, (msg))
