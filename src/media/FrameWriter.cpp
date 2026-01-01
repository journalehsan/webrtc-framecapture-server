#include "media/FrameWriter.h"

#include <filesystem>
#include <iomanip>
#include <sstream>

#include <opencv2/imgcodecs.hpp>

#include "util/Log.h"

namespace media {

FrameWriter::FrameWriter(std::string output_dir, bool write_mp4, std::string mp4_path, double mp4_fps)
    : output_dir_(std::move(output_dir)),
      write_mp4_(write_mp4),
      mp4_path_(std::move(mp4_path)),
      mp4_fps_(mp4_fps) {}

void FrameWriter::EnsureOutputDir() {
  if (dir_ready_) {
    return;
  }
  std::filesystem::create_directories(output_dir_ + "/frames");
  dir_ready_ = true;
}

void FrameWriter::EnsureVideoWriter(const cv::Size& size) {
  if (!write_mp4_) {
    return;
  }
  if (writer_.has_value()) {
    return;
  }
  std::filesystem::create_directories(std::filesystem::path(mp4_path_).parent_path());
  int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
  writer_.emplace(mp4_path_, fourcc, mp4_fps_, size, true);
  if (!writer_->isOpened()) {
    LOG_WARN("Failed to open MP4 writer, disabling video output");
    writer_.reset();
  }
}

void FrameWriter::OnFrame(const cv::Mat& bgr) {
  std::lock_guard<std::mutex> lock(mutex_);
  EnsureOutputDir();
  EnsureVideoWriter(bgr.size());

  std::ostringstream name;
  name << output_dir_ << "/frames/frame_" << std::setw(8) << std::setfill('0')
       << (frame_index_ + 1) << ".png";

  cv::imwrite(name.str(), bgr);
  if (writer_.has_value()) {
    (*writer_) << bgr;
  }
  ++frame_index_;
}

}  // namespace media
