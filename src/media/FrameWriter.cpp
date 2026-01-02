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

void FrameWriter::EnsureOutputDir() {
  if (dir_ready_) {
    return;
  }
  std::filesystem::create_directories(output_dir_ + "/frames");
  dir_ready_ = true;
}

void FrameWriter::EnsureVideoWriter(const cv::Size& size) {
  if (!write_video_) {
    return;
  }
  if (writer_.has_value()) {
    return;
  }
  std::filesystem::create_directories(std::filesystem::path(mp4_path_).parent_path());
  int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  writer_.emplace(mp4_path_, fourcc, mp4_fps_, size, true);
  if (writer_->isOpened()) {
    video_path_ = mp4_path_;
    return;
  }

  writer_.reset();
  std::filesystem::path avi_path = std::filesystem::path(mp4_path_).replace_extension(".avi");
  int avi_fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
  writer_.emplace(avi_path.string(), avi_fourcc, mp4_fps_, size, true);
  if (writer_->isOpened()) {
    video_path_ = avi_path.string();
    LOG_WARN("MP4 writer failed, falling back to " + video_path_);
    return;
  }

  LOG_WARN("Failed to open video writer, disabling video output");
  writer_.reset();
}

void FrameWriter::OnFrame(const cv::Mat& bgr) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (write_images_) {
    EnsureOutputDir();
  }
  EnsureVideoWriter(bgr.size());

  if (write_images_) {
    std::ostringstream name;
    name << output_dir_ << "/frames/frame_" << std::setw(8) << std::setfill('0')
         << (frame_index_ + 1) << ".png";
    cv::imwrite(name.str(), bgr);
  }
  if (writer_.has_value()) {
    (*writer_) << bgr;
  }
  ++frame_index_;
}

void FrameWriter::Close() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (writer_.has_value()) {
    writer_->release();
    writer_.reset();
  }
}

}  // namespace media
