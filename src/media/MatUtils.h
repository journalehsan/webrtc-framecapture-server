#pragma once

#include <opencv2/core.hpp>

#include "api/video/video_frame.h"

namespace media {

class MatUtils {
 public:
  static cv::Mat I420ToBgrMat(const webrtc::VideoFrame& frame);
};

}  // namespace media
