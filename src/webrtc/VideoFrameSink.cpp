#include "webrtc/VideoFrameSink.h"

#include "media/MatUtils.h"

namespace webrtc_wrap {

VideoFrameSink::VideoFrameSink(std::shared_ptr<media::FrameWriter> writer)
    : writer_(std::move(writer)) {}

void VideoFrameSink::OnFrame(const webrtc::VideoFrame& frame) {
  if (!writer_) {
    return;
  }
  cv::Mat bgr = media::MatUtils::I420ToBgrMat(frame);
  writer_->OnFrame(bgr);
}

}  // namespace webrtc_wrap
