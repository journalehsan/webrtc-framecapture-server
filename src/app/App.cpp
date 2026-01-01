#include "app/App.h"

#include "util/Log.h"

namespace app {

App::App(util::Args args) : args_(std::move(args)) {
  frame_writer_ = std::make_shared<media::FrameWriter>(
      args_.output_dir, args_.write_mp4, args_.mp4_path, args_.mp4_fps);
  factory_ = std::make_unique<webrtc_wrap::WebrtcFactory>();
}

bool App::Start() {
  if (!factory_ || !factory_->factory()) {
    LOG_ERROR("WebRTC factory unavailable");
    return false;
  }

  peer_ = std::make_unique<webrtc_wrap::Peer>(factory_->factory(), frame_writer_);
  if (!peer_->Initialize()) {
    return false;
  }

  signaling_ = std::make_unique<signaling::HttpSignalingServer>(
      args_.port,
      [this](const std::string& sdp) { return peer_->HandleOffer(sdp); },
      [this](const std::string& mid, int index, const std::string& cand) {
        peer_->AddIceCandidate(mid, index, cand);
      });
  signaling_->Start();
  return true;
}

void App::Stop() {
  if (signaling_) {
    signaling_->Stop();
  }
}

}  // namespace app
