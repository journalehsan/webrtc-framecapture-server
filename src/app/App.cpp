#include "app/App.h"

#include "util/Log.h"

namespace app {

App::App(util::Args args)
    : args_(std::move(args)),
      frame_writer_(args_.output_dir, args_.write_images, args_.write_video, args_.mp4_path, args_.fps) {}

bool App::Start() {
  receiver_ = std::make_unique<ingest::RtpReceiver>(args_.rtp_url, [this](const cv::Mat& frame) {
    frame_writer_.OnFrame(frame);
  });

  receiver_thread_ = std::thread([this]() {
    if (!receiver_->Run()) {
      LOG_ERROR("RTP receiver stopped with error");
    }
  });

  return true;
}

void App::Stop() {
  if (receiver_) {
    receiver_->Stop();
  }
  if (receiver_thread_.joinable()) {
    receiver_thread_.join();
  }
}

}  // namespace app
