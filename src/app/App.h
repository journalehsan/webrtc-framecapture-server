#pragma once

#include <memory>
#include <thread>

#include "ingest/RtpReceiver.h"
#include "media/FrameWriter.h"
#include "util/Args.h"

namespace app {

// Application orchestrator for RTP capture service.
//
// This class coordinates the RTP receiver and frame writer to create
// the complete capture pipeline. It's the main application component
// managed by main.cpp.
//
// Architecture:
//   Browser → Janus → RTP → RtpReceiver (FFmpeg) → BGR Mat
//                                                  ↓
//                                          FrameWriter (OpenCV)
//                                                  ↓
//                                          PNG frames + MP4/AVI video
//
// Lifecycle:
//   1. Create App with Args configuration
//   2. Call Start() to initialize and start RTP reception
//   3. Call Stop() to gracefully shutdown
//
// Thread model:
//   - RtpReceiver runs in a dedicated thread (blocking Run() call)
//   - FrameWriter is called from the RTP receiver thread
//   - Stop() coordinates thread shutdown
class App {
 public:
  // Create the application with configuration.
  //
  // Param: args - Configuration for RTP source, output paths, flags
  //              See util::Args for details
  explicit App(util::Args args);

  // Start the RTP capture service.
  // This method:
  //   1. Creates RtpReceiver with frame callback to FrameWriter
  //   2. Starts RTP receiver in a dedicated thread
  //   3. Returns immediately (non-blocking)
  //
  // Returns: true if started successfully, false otherwise
  // Side effects:
  //   - Initializes RtpReceiver and FrameWriter
  //   - Spawns receiver_thread_ for RTP reception
  //   - Frames flow through: RTP → RtpReceiver → FrameWriter → disk
  bool Start();

  // Stop the RTP capture service and cleanup.
  // This method:
  //   1. Signals RtpReceiver to stop (thread-safe)
  //   2. Waits for receiver thread to finish (join)
  //   3. Closes FrameWriter to finalize video file
  //
  // Important: Must be called to properly close video file.
  //            Video file is invalid until Close() is called.
  //
  // Thread-safe: can be called from any thread
  // Side effects:
  //   - Stops RTP reception
  //   - Waits for thread completion (blocking)
  //   - Finalizes and closes video file
  void Stop();

 private:
  // Configuration from command-line arguments
  util::Args args_;

  // Frame writer: receives decoded frames and writes to disk
  // Thread-safe: OnFrame() and Close() can be called concurrently
  media::FrameWriter frame_writer_;

  // RTP receiver: receives packets, decodes to BGR Mat
  // Runs in a dedicated thread; callback runs on that thread
  std::unique_ptr<ingest::RtpReceiver> receiver_;

  // Thread running the RTP receiver
  // Created in Start(), joined in Stop()
  std::thread receiver_thread_;
};

}  // namespace app
