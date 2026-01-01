#include <cassert>
#include <filesystem>

#include <opencv2/core.hpp>

#include "media/FrameWriter.h"

int main() {
  std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "webrtc_framewriter_test";
  std::filesystem::remove_all(temp_dir);

  media::FrameWriter writer(temp_dir.string(), true, false, (temp_dir / "capture.mp4").string(), 30.0);
  cv::Mat image(2, 2, CV_8UC3, cv::Scalar(0, 0, 255));
  writer.OnFrame(image);

  std::filesystem::path output = temp_dir / "frames" / "frame_00000001.png";
  assert(std::filesystem::exists(output));

  return 0;
}
