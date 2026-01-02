// RTP Capture Service Entry Point
//
// This is the main entry point for the WebRTC RTP capture service. It:
// 1. Initializes the logging system with a prefix for identification
// 2. Parses command-line arguments for configuration
// 3. Starts the App which orchestrates RTP receiving and frame writing
// 4. Handles graceful shutdown via SIGINT/SIGTERM signals
//
// The application runs in a dedicated thread for RTP reception, while the main
// thread waits for shutdown signals. This ensures proper resource cleanup.
//
// Architecture:
//   Browser → Janus → RTP → FFmpeg/libav → OpenCV → disk
//
// See src/app/App.h for orchestration details.

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

#include "app/App.h"
#include "util/Args.h"
#include "util/Log.h"

namespace {
// Global flag for controlling the main event loop.
// Set to false by signal handlers to trigger graceful shutdown.
std::atomic<bool> g_running{true};

// Signal handler for SIGINT and SIGTERM.
// Sets the running flag to false, which causes the main loop to exit
// and initiate cleanup of the RTP receiver and frame writer.
void HandleSignal(int) {
  g_running = false;
}
}

int main(int argc, char** argv) {
  // Initialize logging with a prefix for easy log filtering
  util::Log::Instance().SetPrefix("rtp-capture");
  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  // Parse command-line arguments (RTP URL, output directory, flags)
  util::Args args = util::ParseArgs(argc, argv);

  // Create and start the application
  // This initializes the RTP receiver and frame writer with parsed arguments
  app::App app(args);
  if (!app.Start()) {
    LOG_ERROR("Failed to start app");
    return 1;
  }

  // Main event loop: wait for shutdown signal
  // The RTP receiver runs in a separate thread, so this loop just
  // waits for the user to press Ctrl+C or for the service to be terminated
  LOG_INFO("Service running. Press Ctrl+C to stop.");
  while (g_running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // Graceful shutdown: stop RTP receiver, close video writer, cleanup resources
  app.Stop();
  return 0;
}
