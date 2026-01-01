#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

#include "app/App.h"
#include "util/Args.h"
#include "util/Log.h"

namespace {
std::atomic<bool> g_running{true};

void HandleSignal(int) {
  g_running = false;
}
}

int main(int argc, char** argv) {
  util::Log::Instance().SetPrefix("rtp-capture");
  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  util::Args args = util::ParseArgs(argc, argv);
  app::App app(args);
  if (!app.Start()) {
    LOG_ERROR("Failed to start app");
    return 1;
  }

  LOG_INFO("Service running. Press Ctrl+C to stop.");
  while (g_running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  app.Stop();
  return 0;
}
