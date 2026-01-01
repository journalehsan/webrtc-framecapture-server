#pragma once

#include <memory>

#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"

#include "media/FrameWriter.h"

namespace webrtc_wrap {

class VideoFrameSink : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  explicit VideoFrameSink(std::shared_ptr<media::FrameWriter> writer);

  void OnFrame(const webrtc::VideoFrame& frame) override;

 private:
  std::shared_ptr<media::FrameWriter> writer_;
};

}  // namespace webrtc_wrap
