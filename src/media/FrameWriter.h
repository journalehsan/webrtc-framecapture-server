#pragma once

#include <mutex>
#include <optional>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

namespace media {

// Frame writer using OpenCV.
//
// This class receives decoded frames (as OpenCV Mat) and writes them to:
//   1. Individual PNG files (e.g., frame_00000001.png)
//   2. A video file (MP4 or AVI fallback)
//
// Why OpenCV?
//   - Simple API for writing images and videos
//   - Handles multiple video codecs automatically
//   - Cross-platform support
//
// Usage:
//   1. Create with configuration (output paths, flags)
//   2. Call OnFrame() for each decoded frame
//   3. Call Close() to finalize video file and cleanup
//
// Thread safety:
//   - OnFrame() and Close() are thread-safe
//   - Uses a mutex to protect internal state
//
// Frame numbering:
//   - Starts at 1, not 0 (human-friendly)
//   - 8-digit zero-padded (frame_00000001.png)
class FrameWriter {
 public:
  // Create a frame writer with the specified configuration.
  //
  // Param: output_dir - Base directory for PNG frames (created if needed)
  //                     Frames are written to "<output_dir>/frames/"
  // Param: write_images - If true, save each frame as PNG
  // Param: write_video - If true, encode frames into video file
  // Param: mp4_path - Full path for video output (e.g., "out/capture.mp4")
  //                   Parent directories are created automatically
  // Param: mp4_fps - Frame rate for video encoding (doesn't affect capture rate)
  FrameWriter(std::string output_dir,
              bool write_images,
              bool write_video,
              std::string mp4_path,
              double mp4_fps);

  // Process a decoded frame and write to disk.
  // This method:
  //   1. Ensures output directory exists
  //   2. Initializes video writer on first call
  //   3. Writes frame as PNG (if enabled)
  //   4. Writes frame to video (if enabled)
  //   5. Increments frame counter
  //
  // Thread-safe: acquires mutex for entire operation.
  //
  // Param: bgr - Frame in BGR format (3 channels, 8-bit)
  // Side effects:
  //   - Creates directories if they don't exist
  //   - Initializes video writer on first frame
  //   - Writes to disk (may block on I/O)
  void OnFrame(const cv::Mat& bgr);

  // Finalize video file and cleanup resources.
  // This method:
  //   1. Closes the video writer (if open)
  //   2. Flushes any buffered video data
  //   3. Resets the video writer
  //
  // Important: Must be called to properly close the video file.
  //            Video file is invalid until Close() is called.
  //
  // Thread-safe: acquires mutex for entire operation.
  void Close();

 private:
  // Ensure the output directory exists.
  // Creates "<output_dir_>/frames/" if it doesn't exist.
  // Uses a flag (dir_ready_) to avoid redundant filesystem checks.
  //
  // Not thread-safe internally, but only called from OnFrame() which holds mutex.
  void EnsureOutputDir();

  // Ensure the video writer is initialized and ready.
  // On first call, attempts to:
  //   1. Create parent directories for mp4_path_
  //   2. Open MP4 writer with H.264 codec (fourcc='mp4v')
  //   3. If MP4 fails, fallback to AVI with MJPG codec (more compatible)
  //   4. If both fail, disable video output and log warning
  //
  // Subsequent calls are no-ops if writer_ is already open.
  //
  // Not thread-safe internally, but only called from OnFrame() which holds mutex.
  //
  // Param: size - Frame dimensions (width, height) for video encoder
  // Side effects:
  //   - Creates directories if needed
  //   - Opens video file for writing
  //   - May update video_path_ if fallback to AVI occurs
  void EnsureVideoWriter(const cv::Size& size);

  // Mutex protecting all internal state and I/O operations
  std::mutex mutex_;

  // Configuration
  std::string output_dir_;    // Base directory for PNG frames
  bool write_images_;         // Enable PNG frame output
  bool write_video_;          // Enable video output
  std::string mp4_path_;      // Configured video path (may be MP4 or AVI)
  std::string video_path_;    // Actual video path (may change on fallback)
  double mp4_fps_;            // Video frame rate for encoding

  // State
  size_t frame_index_ = 0;    // Counter for frame numbering (starts at 1 in output)
  std::optional<cv::VideoWriter> writer_;  // Video writer (optional if disabled)
  bool dir_ready_ = false;    // Flag: true if output directory exists
};

}  // namespace media
