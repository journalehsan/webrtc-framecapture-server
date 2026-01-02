#pragma once

#include <atomic>
#include <functional>
#include <string>

#include <opencv2/core.hpp>

namespace ingest {

// RTP receiver using FFmpeg/libav.
//
// This class receives RTP packets, decodes them using FFmpeg, and converts
// the decoded frames to OpenCV Mat format for downstream processing.
//
// Why FFmpeg?
//   - Handles various RTP payloads (VP8, H.264, etc.)
//   - Manages packet reassembly and decoder state
//   - Robust handling of network jitter and packet loss
//
// Usage:
//   1. Create with RTP URL and frame callback
//   2. Call Run() to start the receive loop (blocking)
//   3. Call Stop() from another thread to gracefully shutdown
//
// Thread model:
//   - Run() blocks the calling thread for the duration of the stream
//   - Stop() is thread-safe and can be called from any thread
//   - Frame callback is invoked on the Run() thread
//
// Architecture:
//   Janus → RTP (UDP) → FFmpeg libavformat → libavcodec → swscale → BGR Mat
class RtpReceiver {
 public:
  // Callback type invoked for each decoded frame.
  // The callback receives a cv::Mat in BGR format (3 channels, 8-bit).
  // The Mat is reused for each frame; copy it if you need to retain data.
  using FrameCallback = std::function<void(const cv::Mat&)>;

  // Create an RTP receiver with the given source and callback.
  //
  // Param: url - RTP source URL or SDP file path
  //               Examples:
  //                 "rtp://0.0.0.0:5004?protocol_whitelist=file,udp,rtp"
  //                 "/app/config/rtp.sdp"
  // Param: on_frame - Callback invoked for each decoded frame
  //                   The callback runs on the Run() thread
  RtpReceiver(std::string url, FrameCallback on_frame);

  // Start the RTP receiver loop.
  // This is a blocking call that:
  //   1. Opens the RTP stream using FFmpeg
  //   2. Finds and opens the video codec
  //   3. Reads packets, decodes frames, and invokes the callback
  //   4. Returns when the stream ends or Stop() is called
  //
  // Thread model: blocks the calling thread; typically run in a dedicated thread
  //
  // Returns: true on success, false if initialization failed
  // Side effects:
  //   - Initializes FFmpeg network subsystem
  //   - Allocates FFmpeg contexts and buffers
  //   - Invokes on_frame_ callback for each decoded frame
  bool Run();

  // Request graceful shutdown of the receiver.
  // Thread-safe: can be called from any thread.
  // Sets a flag checked by the Run() loop, causing it to exit cleanly.
  //
  // After calling Stop(), the Run() method will:
  //   - Stop reading new packets
  //   - Flush any pending frames
  //   - Release FFmpeg resources
  //   - Return to the caller
  void Stop();

 private:
  // RTP source URL or SDP file path
  std::string url_;

  // Callback invoked for each decoded frame
  FrameCallback on_frame_;

  // Flag controlling the Run() loop.
  // Atomic for thread-safe Stop() from another thread.
  std::atomic<bool> running_{false};
};

}  // namespace ingest
