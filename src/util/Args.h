#pragma once

#include <string>

namespace util {

struct Args {
  int port = 8080;
  std::string output_dir = "out";
  bool write_mp4 = false;
  std::string mp4_path = "out/capture.mp4";
  double mp4_fps = 30.0;
};

Args ParseArgs(int argc, char** argv);

}  // namespace util
