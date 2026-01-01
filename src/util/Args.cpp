#include "util/Args.h"

#include <cstdlib>

#include "util/Log.h"

namespace util {

Args ParseArgs(int argc, char** argv) {
  Args args;
  for (int i = 1; i < argc; ++i) {
    std::string key = argv[i];
    if (key == "--rtp-url" && i + 1 < argc) {
      args.rtp_url = argv[++i];
    } else if ((key == "--out" || key == "--output") && i + 1 < argc) {
      args.output_dir = argv[++i];
      args.mp4_path = args.output_dir + "/capture.mp4";
    } else if (key == "--write-images" && i + 1 < argc) {
      args.write_images = std::atoi(argv[++i]) != 0;
    } else if (key == "--write-video" && i + 1 < argc) {
      args.write_video = std::atoi(argv[++i]) != 0;
    } else if (key == "--fps" && i + 1 < argc) {
      args.fps = std::atof(argv[++i]);
    } else if (key == "--mp4" && i + 1 < argc) {
      args.mp4_path = argv[++i];
      args.write_video = true;
    } else if (key == "--help") {
      LOG_INFO("Usage: --rtp-url <url|sdp> --out <dir> --write-images 1|0 --write-video 1|0 --fps <fps> --mp4 <path>");
    } else {
      LOG_WARN("Unknown arg: " + key);
    }
  }
  return args;
}

}  // namespace util
