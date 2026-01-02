#include "media/FrameWriter.h"

#include <filesystem>
#include <iomanip>
#include <sstream>

#include <opencv2/imgcodecs.hpp>

#include "util/Log.h"

namespace media {

FrameWriter::FrameWriter(std::string output_dir,
                         bool write_images,
                         bool write_video,
                         std::string mp4_path,
                         double mp4_fps)
    : output_dir_(std::move(output_dir)),
      write_images_(write_images),
      write_video_(write_video),
      mp4_path_(std::move(mp4_path)),
      video_path_(mp4_path_),
      mp4_fps_(mp4_fps) {}

// Ensure the frames output directory exists.
// Creates "<output_dir_>/frames/" if it doesn't exist.
// Uses dir_ready_ flag to avoid redundant filesystem checks.
//
// This is called lazily on the first OnFrame() call to avoid
// filesystem operations during startup (especially in tests).
void FrameWriter::EnsureOutputDir() {
  if (dir_ready_) {
    return;
  }
  std::filesystem::create_directories(output_dir_ + "/frames");
  dir_ready_ = true;
}

// Initialize the video writer on first use.
// This method handles the OpenCV video writer initialization with fallback:
//
// Strategy:
//   1. Try MP4 with H.264 codec (fourcc='mp4v')
//   2. If that fails, try AVI with MJPG codec (more compatible)
//   3. If that fails, disable video output
//
// Why fallback? MP4 encoding requires codec support (libx264 or similar).
// Some systems only have MJPG support, so AVI is a safe fallback.
//
// Param: size - Frame dimensions for the video encoder
// Side effects:
//   - Creates parent directories for video file
//   - Opens video file for writing
//   - May update video_path_ if fallback to AVI occurs
//   - Logs warnings if video output is disabled
void FrameWriter::EnsureVideoWriter(const cv::Size& size) {
  if (!write_video_) {
    return;
  }
  if (writer_.has_value()) {
    return;
  }

  // Create parent directories for video file
  std::filesystem::create_directories(std::filesystem::path(mp4_path_).parent_path());

  // Try MP4 with H.264 codec
  int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  writer_.emplace(mp4_path_, fourcc, mp4_fps_, size, true);
  if (writer_->isOpened()) {
    video_path_ = mp4_path_;
    return;
  }

  // MP4 failed, fallback to AVI with MJPG
  // MJPG is widely supported and doesn't require H.264 encoding
  writer_.reset();
  std::filesystem::path avi_path = std::filesystem::path(mp4_path_).replace_extension(".avi");
  int avi_fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
  writer_.emplace(avi_path.string(), avi_fourcc, mp4_fps_, size, true);
  if (writer_->isOpened()) {
    video_path_ = avi_path.string();
    LOG_WARN("MP4 writer failed, falling back to " + video_path_);
    return;
  }

  // Both MP4 and AVI failed, disable video output
  LOG_WARN("Failed to open video writer, disabling video output");
  writer_.reset();
}

// Process a frame and write to disk.
// This method handles both PNG frame output and video encoding.
//
// For each frame:
//   1. Create output directory if needed (lazy init)
//   2. Initialize video writer on first frame (lazy init)
//   3. Write frame as PNG: "frame_00000001.png" (8-digit zero-padded)
//   4. Write frame to video (if enabled)
//   5. Increment frame counter
//
// Frame numbering: starts at 1 in output (frame_00000001.png)
//                   but internal counter starts at 0
//
// Thread-safe: acquires mutex_ for entire operation.
//
// Param: bgr - Frame in BGR format (3 channels, 8-bit)
// Side effects:
//   - Writes PNG file to disk (I/O)
//   - Appends frame to video file (I/O)
//   - Creates directories if needed
void FrameWriter::OnFrame(const cv::Mat& bgr) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (write_images_) {
    EnsureOutputDir();
  }
  EnsureVideoWriter(bgr.size());

  // Write frame as PNG file
  if (write_images_) {
    std::ostringstream name;
    name << output_dir_ << "/frames/frame_" << std::setw(8) << std::setfill('0')
         << (frame_index_ + 1) << ".png";
    cv::imwrite(name.str(), bgr);
  }

  // Write frame to video
  if (writer_.has_value()) {
    (*writer_) << bgr;
  }
  ++frame_index_;
}

// Finalize video file and cleanup.
// This method:
//   1. Closes the video writer, which flushes any buffered data
//   2. Releases the video file handle
//   3. Resets the optional writer_ to empty
//
// Important: The video file is incomplete until Close() is called.
//            OpenCV VideoWriter requires explicit release to finalize.
//
// Thread-safe: acquires mutex_ for entire operation.
// Side effects:
//   - Flushes video data to disk
//   - Closes video file
//   - Resets writer_ state (can be re-initialized if needed)
void FrameWriter::Close() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (writer_.has_value()) {
    writer_->release();
    writer_.reset();
  }
}

}  // namespace media
