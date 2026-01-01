#include "util/Args.h"

#include <cstdlib>

#include "util/Log.h"

namespace util {

Args ParseArgs(int argc, char** argv) {
  Args args;
  for (int i = 1; i < argc; ++i) {
    std::string key = argv[i];
    if (key == "--port" && i + 1 < argc) {
      args.port = std::atoi(argv[++i]);
    } else if (key == "--output" && i + 1 < argc) {
      args.output_dir = argv[++i];
      args.mp4_path = args.output_dir + "/capture.mp4";
    } else if (key == "--write-mp4") {
      args.write_mp4 = true;
    } else if (key == "--mp4" && i + 1 < argc) {
      args.write_mp4 = true;
      args.mp4_path = argv[++i];
    } else if (key == "--mp4-fps" && i + 1 < argc) {
      args.mp4_fps = std::atof(argv[++i]);
    } else if (key == "--help") {
      LOG_INFO("Usage: --port <port> --output <dir> --write-mp4 --mp4 <path> --mp4-fps <fps>");
    } else {
      LOG_WARN("Unknown arg: " + key);
    }
  }
  return args;
}

}  // namespace util
