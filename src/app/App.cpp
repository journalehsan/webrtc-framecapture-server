#include "app/App.h"

#include "util/Log.h"

namespace app {

App::App(util::Args args)
    : args_(std::move(args)),
      frame_writer_(args_.output_dir, args_.write_images, args_.write_video, args_.mp4_path, args_.fps) {}

// Start the RTP capture service.
//
// This method sets up the complete capture pipeline:
//
// 1. Create RtpReceiver with a lambda callback
//    - The callback receives BGR frames from the RTP stream
//    - It forwards frames to FrameWriter for disk I/O
//
// 2. Start RTP receiver in a dedicated thread
//    - The receiver_thread_ calls receiver_->Run() (blocking)
//    - Run() loops until Stop() is called or stream ends
//    - Each decoded frame invokes the callback
//
// The frame flow:
//   RTP (UDP) → FFmpeg decode → BGR Mat → callback → FrameWriter → disk
//
// Thread model:
//   - Main thread: calls Start() and continues
//   - receiver_thread_: blocks on receiver_->Run()
//   - FrameWriter.OnFrame() runs on receiver_thread_
//
// Returns: true (always; errors are logged)
// Side effects:
//   - Creates RtpReceiver and starts reception
//   - Frames begin flowing through the pipeline
bool App::Start() {
  // Create RTP receiver with frame callback
  // The lambda captures 'this' to call frame_writer_
  receiver_ = std::make_unique<ingest::RtpReceiver>(args_.rtp_url, [this](const cv::Mat& frame) {
    frame_writer_.OnFrame(frame);
  });

  // Start receiver in dedicated thread
  // Run() is blocking, so it needs its own thread
  receiver_thread_ = std::thread([this]() {
    if (!receiver_->Run()) {
      LOG_ERROR("RTP receiver stopped with error");
    }
  });

  return true;
}

// Stop the RTP capture service and cleanup.
//
// This method performs a graceful shutdown sequence:
//
// 1. Signal RTP receiver to stop
//    - receiver_->Stop() sets atomic flag
//    - The Run() loop checks this flag and exits
//
// 2. Wait for receiver thread to finish
//    - receiver_thread_.join() blocks until thread exits
//    - This ensures all frames are processed before continuing
//
// 3. Finalize frame writer
//    - frame_writer_.Close() releases video file
//    - Video file is invalid until Close() is called
//
// Thread safety:
//   - Stop() can be called from any thread (e.g., signal handler)
//   - FrameWriter is thread-safe, so concurrent OnFrame() during Close() is OK
//
// Side effects:
//   - Stops RTP packet reception
//   - Waits for thread completion (blocking)
//   - Finalizes video file on disk
//   - Cleans up RTP receiver resources
void App::Stop() {
  if (receiver_) {
    receiver_->Stop();
  }
  if (receiver_thread_.joinable()) {
    receiver_thread_.join();
  }
  frame_writer_.Close();
}

}  // namespace app
