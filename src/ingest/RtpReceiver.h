#pragma once

#include <atomic>
#include <functional>
#include <string>

#include <opencv2/core.hpp>

namespace ingest {

class RtpReceiver {
 public:
  using FrameCallback = std::function<void(const cv::Mat&)>;

  RtpReceiver(std::string url, FrameCallback on_frame);

  bool Run();
  void Stop();

 private:
  std::string url_;
  FrameCallback on_frame_;
  std::atomic<bool> running_{false};
};

}  // namespace ingest
