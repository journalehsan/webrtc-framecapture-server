#include "util/Args.h"

#include <cstdlib>

#include "util/Log.h"

namespace util {

// Parse command-line arguments and return an Args struct.
//
// This function iterates through argv and sets the corresponding fields
// in the Args struct. It uses a simple linear scan rather than getopt
// to avoid external dependencies.
//
// Argument handling:
//   --out also updates --mp4_path to "<dir>/capture.mp4" for convenience
//   --mp4 enables write_video automatically
//   Unknown arguments are logged as warnings (not errors)
//   --help prints usage and returns with default args
//
// Param: argc - Number of command-line arguments (from main())
// Param: argv - Array of argument strings (from main())
// Returns: Args struct with defaults overridden by provided arguments
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
