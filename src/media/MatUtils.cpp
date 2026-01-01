#include "media/MatUtils.h"

#include <cstring>

#include <opencv2/imgproc.hpp>

#include "api/video/i420_buffer.h"

namespace media {

cv::Mat MatUtils::I420ToBgrMat(const webrtc::VideoFrame& frame) {
  rtc::scoped_refptr<webrtc::I420BufferInterface> buffer = frame.video_frame_buffer()->ToI420();
  const int width = buffer->width();
  const int height = buffer->height();

  cv::Mat i420(height + height / 2, width, CV_8UC1);

  uint8_t* y_plane = i420.data;
  uint8_t* u_plane = i420.data + width * height;
  uint8_t* v_plane = u_plane + (width * height) / 4;

  for (int y = 0; y < height; ++y) {
    std::memcpy(y_plane + y * width, buffer->DataY() + y * buffer->StrideY(), width);
  }
  for (int y = 0; y < height / 2; ++y) {
    std::memcpy(u_plane + y * (width / 2), buffer->DataU() + y * buffer->StrideU(), width / 2);
    std::memcpy(v_plane + y * (width / 2), buffer->DataV() + y * buffer->StrideV(), width / 2);
  }

  cv::Mat bgr;
  cv::cvtColor(i420, bgr, cv::COLOR_YUV2BGR_I420);
  return bgr;
}

}  // namespace media
