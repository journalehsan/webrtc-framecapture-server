#pragma once

#include <string>

namespace util {

struct Args {
  std::string rtp_url = "rtp://0.0.0.0:5004?protocol_whitelist=file,udp,rtp";
  std::string output_dir = "out";
  bool write_images = true;
  bool write_video = true;
  std::string mp4_path = "out/capture.mp4";
  double fps = 30.0;
};

Args ParseArgs(int argc, char** argv);

}  // namespace util
