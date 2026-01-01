#pragma once

#include <mutex>
#include <optional>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

namespace media {

class FrameWriter {
 public:
  FrameWriter(std::string output_dir,
              bool write_images,
              bool write_video,
              std::string mp4_path,
              double mp4_fps);

  void OnFrame(const cv::Mat& bgr);

 private:
  void EnsureOutputDir();
  void EnsureVideoWriter(const cv::Size& size);

  std::mutex mutex_;
  std::string output_dir_;
  bool write_images_;
  bool write_video_;
  std::string mp4_path_;
  double mp4_fps_;
  size_t frame_index_ = 0;
  std::optional<cv::VideoWriter> writer_;
  bool dir_ready_ = false;
};

}  // namespace media
