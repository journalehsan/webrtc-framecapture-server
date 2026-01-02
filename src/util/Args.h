#pragma once

#include <string>

namespace util {

// Configuration arguments for the RTP capture service.
//
// These settings control:
// - Where to receive RTP packets (URL/SDP file path)
// - Where to save output files
// - Whether to write individual frames and/or video
// - Video output format and frame rate
//
// Defaults are set to work with the standard Docker setup:
//   RTP from Janus on port 5004, output to "out/" directory.
//
// See util/ParseArgs() for command-line argument parsing.
struct Args {
  // RTP source URL or path to SDP file.
  // Examples:
  //   "rtp://0.0.0.0:5004?protocol_whitelist=file,udp,rtp" - Receive RTP on UDP 5004
  //   "/app/config/rtp.sdp" - Read SDP file (common in Docker)
  // The protocol_whitelist is required by FFmpeg for security.
  std::string rtp_url = "rtp://0.0.0.0:5004?protocol_whitelist=file,udp,rtp";

  // Base directory for output files.
  // PNG frames are written to "<output_dir>/frames/"
  // Video is written to a path derived from mp4_path (often within output_dir)
  std::string output_dir = "out";

  // If true, saves each decoded frame as a PNG file.
  // Frames are named "frame_00000001.png", "frame_00000002.png", etc.
  // Useful for frame-by-frame analysis or machine learning datasets.
  bool write_images = true;

  // If true, encodes frames into a video file.
  // Tries MP4 first (H.264), falls back to AVI (MJPG) if MP4 encoding unavailable.
  bool write_video = true;

  // Path to the output video file.
  // Can be absolute or relative to the working directory.
  // If relative, parent directories are created automatically.
  std::string mp4_path = "out/capture.mp4";

  // Frame rate for video output (frames per second).
  // This affects video file encoding, not the actual capture rate
  // (capture rate is determined by incoming RTP stream).
  double fps = 30.0;
};

// Parse command-line arguments into an Args struct.
//
// Supported arguments:
//   --rtp-url <url|sdp>     RTP source URL or SDP file path
//   --out, --output <dir>  Output directory (sets mp4_path to <dir>/capture.mp4)
//   --write-images 1|0     Enable/disable PNG frame output
//   --write-video 1|0      Enable/disable video output
//   --fps <fps>            Video frame rate
//   --mp4 <path>           Override MP4 output path (enables video)
//   --help                 Show usage message
//
// Args parsing uses a simple loop, not a library like getopt, to avoid
// external dependencies. Unknown arguments are logged as warnings.
//
// Returns: Args struct with defaults overridden by provided arguments
Args ParseArgs(int argc, char** argv);

}  // namespace util
