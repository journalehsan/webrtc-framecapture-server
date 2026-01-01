#include <cassert>
#include <cmath>
#include <cstring>

#include <opencv2/core.hpp>

#include "api/video/i420_buffer.h"
#include "api/video/video_frame.h"

#include "media/MatUtils.h"

int main() {
  const int width = 4;
  const int height = 4;
  rtc::scoped_refptr<webrtc::I420Buffer> buffer = webrtc::I420Buffer::Create(width, height);

  for (int y = 0; y < height; ++y) {
    std::memset(buffer->MutableDataY() + y * buffer->StrideY(), 128, width);
  }
  for (int y = 0; y < height / 2; ++y) {
    std::memset(buffer->MutableDataU() + y * buffer->StrideU(), 128, width / 2);
    std::memset(buffer->MutableDataV() + y * buffer->StrideV(), 128, width / 2);
  }

  webrtc::VideoFrame frame = webrtc::VideoFrame::Builder()
                                 .set_video_frame_buffer(buffer)
                                 .set_timestamp_ms(0)
                                 .build();

  cv::Mat bgr = media::MatUtils::I420ToBgrMat(frame);
  assert(bgr.rows == height);
  assert(bgr.cols == width);
  cv::Vec3b pixel = bgr.at<cv::Vec3b>(0, 0);
  for (int c = 0; c < 3; ++c) {
    assert(std::abs(pixel[c] - 128) < 5);
  }

  return 0;
}
