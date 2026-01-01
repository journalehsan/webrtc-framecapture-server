#include "util/Log.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace util {

Log& Log::Instance() {
  static Log instance;
  return instance;
}

void Log::SetPrefix(const std::string& prefix) {
  std::lock_guard<std::mutex> lock(mutex_);
  prefix_ = prefix;
}

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
