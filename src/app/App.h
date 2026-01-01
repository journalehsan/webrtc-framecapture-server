#pragma once

#include <memory>

#include "media/FrameWriter.h"
#include "signaling/HttpSignalingServer.h"
#include "util/Args.h"
#include "webrtc/Peer.h"
#include "webrtc/WebrtcFactory.h"

namespace app {

class App {
 public:
  explicit App(util::Args args);

  bool Start();
  void Stop();

 private:
  util::Args args_;
  std::shared_ptr<media::FrameWriter> frame_writer_;
  std::unique_ptr<webrtc_wrap::WebrtcFactory> factory_;
  std::unique_ptr<webrtc_wrap::Peer> peer_;
  std::unique_ptr<signaling::HttpSignalingServer> signaling_;
};

}  // namespace app
