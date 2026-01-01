#pragma once

#include <memory>
#include <thread>

#include "ingest/RtpReceiver.h"
#include "media/FrameWriter.h"
#include "util/Args.h"

namespace app {

class App {
 public:
  explicit App(util::Args args);
  bool Start();
  void Stop();

 private:
  util::Args args_;
  media::FrameWriter frame_writer_;
  std::unique_ptr<ingest::RtpReceiver> receiver_;
  std::thread receiver_thread_;
};

}  // namespace app
