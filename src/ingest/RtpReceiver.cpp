#include "ingest/RtpReceiver.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <sstream>

#include "util/Log.h"

namespace ingest {
namespace {

// Convert FFmpeg error code to human-readable string.
// FFmpeg returns negative error codes; this converts them to messages.
// Example: -109 → "Invalid data found when processing input"
//
// Param: err - FFmpeg error code (negative value from FFmpeg function)
// Returns: Human-readable error message string
std::string AvErrorToString(int err) {
  char buffer[AV_ERROR_MAX_STRING_SIZE];
  av_strerror(err, buffer, sizeof(buffer));
  return buffer;
}

}  // namespace

RtpReceiver::RtpReceiver(std::string url, FrameCallback on_frame)
    : url_(std::move(url)), on_frame_(std::move(on_frame)) {}

// Main RTP receive and decode loop.
//
// This method orchestrates the entire FFmpeg pipeline:
//   1. Open RTP input (network or SDP file)
//   2. Detect stream format and codec
//   3. Initialize decoder context
//   4. Read packets, decode to frames
//   5. Convert pixel format to BGR (OpenCV format)
//   6. Invoke callback with each decoded frame
//
// FFmpeg context cleanup:
//   - All allocated resources are freed on error or exit
//   - Follows FFmpeg resource allocation pattern
//
// Returns: true on successful shutdown, false on initialization error
// Side effects:
//   - Initializes FFmpeg network subsystem
//   - Runs on the calling thread (blocking)
//   - Invokes on_frame_ callback for each decoded frame
bool RtpReceiver::Run() {
  running_ = true;
  avformat_network_init();

  AVFormatContext* format_ctx = nullptr;
  AVDictionary* options = nullptr;

  // Configure FFmpeg input options:
  // - protocol_whitelist: restrict to safe protocols (file, udp, rtp)
  // - analyzeduration: time to analyze stream format (10 seconds)
  // - probesize: bytes to analyze for codec detection (5 MB)
  // These defaults help with RTP streams that may have delayed I-frames
  av_dict_set(&options, "protocol_whitelist", "file,udp,rtp", 0);
  av_dict_set(&options, "analyzeduration", "10000000", 0);
  av_dict_set(&options, "probesize", "5000000", 0);
  int ret = avformat_open_input(&format_ctx, url_.c_str(), nullptr, &options);
  av_dict_free(&options);
  if (ret < 0) {
    LOG_ERROR("Failed to open input: " + AvErrorToString(ret));
    return false;
  }

  // Analyze stream to find codec parameters
  ret = avformat_find_stream_info(format_ctx, nullptr);
  if (ret < 0) {
    LOG_ERROR("Failed to find stream info: " + AvErrorToString(ret));
    avformat_close_input(&format_ctx);
    return false;
  }

  // Find the video stream in the input (could be multiple streams: audio, video, etc.)
  int video_stream_index = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (video_stream_index < 0) {
    LOG_ERROR("No video stream found: " + AvErrorToString(video_stream_index));
    avformat_close_input(&format_ctx);
    return false;
  }

  // Get codec parameters from the stream
  AVStream* video_stream = format_ctx->streams[video_stream_index];
  const AVCodec* codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
  if (!codec) {
    LOG_ERROR("No decoder for codec id: " + std::to_string(video_stream->codecpar->codec_id));
    avformat_close_input(&format_ctx);
    return false;
  }

  // Allocate codec context and copy parameters from stream
  AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx) {
    LOG_ERROR("Failed to allocate codec context");
    avformat_close_input(&format_ctx);
    return false;
  }

  ret = avcodec_parameters_to_context(codec_ctx, video_stream->codecpar);
  if (ret < 0) {
    LOG_ERROR("Failed to copy codec parameters: " + AvErrorToString(ret));
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    return false;
  }

  // Open the decoder
  ret = avcodec_open2(codec_ctx, codec, nullptr);
  if (ret < 0) {
    LOG_ERROR("Failed to open codec: " + AvErrorToString(ret));
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    return false;
  }

  // Prepare swscale context for pixel format conversion
  // FFmpeg decodes to YUV (usually), OpenCV needs BGR
  SwsContext* sws_ctx = nullptr;

  AVPacket* packet = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  if (!packet || !frame) {
    LOG_ERROR("Failed to allocate packet/frame");
    av_packet_free(&packet);
    av_frame_free(&frame);
    sws_freeContext(sws_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    return false;
  }

  // OpenCV Mat to hold the BGR frame (reused for each frame)
  cv::Mat bgr;
  int last_width = 0;
  int last_height = 0;

  // Main receive loop: read packets, decode, convert, callback
  while (running_) {
    ret = av_read_frame(format_ctx, packet);
    if (ret == AVERROR(EAGAIN)) {
      // Temporary failure, try again
      continue;
    }
    if (ret < 0) {
      // Stream ended or error
      LOG_INFO("Stream ended or error: " + AvErrorToString(ret));
      break;
    }

    // Only process packets from the video stream
    if (packet->stream_index == video_stream_index) {
      // Send packet to decoder
      ret = avcodec_send_packet(codec_ctx, packet);
      if (ret < 0) {
        LOG_WARN("Failed to send packet: " + AvErrorToString(ret));
      } else {
        // Receive all frames from this packet (may be 0 or multiple)
        while (ret >= 0) {
          ret = avcodec_receive_frame(codec_ctx, frame);
          if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            // Need more input or end of stream
            break;
          }
          if (ret < 0) {
            // Decode error
            LOG_WARN("Failed to decode frame: " + AvErrorToString(ret));
            break;
          }

          // Validate frame dimensions
          const int width = frame->width;
          const int height = frame->height;
          if (width <= 0 || height <= 0) {
            continue;
          }

          // Reinitialize swscale context if frame size changed
          // This can happen if the stream switches resolution
          if (width != last_width || height != last_height || !sws_ctx) {
            if (sws_ctx) {
              sws_freeContext(sws_ctx);
            }
            // Create swscale context: convert from source format to BGR24
            sws_ctx = sws_getContext(width,
                                     height,
                                     static_cast<AVPixelFormat>(frame->format),
                                     width,
                                     height,
                                     AV_PIX_FMT_BGR24,
                                     SWS_BILINEAR,
                                     nullptr,
                                     nullptr,
                                     nullptr);
            if (!sws_ctx) {
              LOG_ERROR("Failed to create swscale context");
              av_packet_unref(packet);
              av_packet_free(&packet);
              av_frame_free(&frame);
              avcodec_free_context(&codec_ctx);
              avformat_close_input(&format_ctx);
              return false;
            }
            // Allocate OpenCV Mat for BGR data (reused)
            bgr = cv::Mat(height, width, CV_8UC3);
            last_width = width;
            last_height = height;
          }

          // Prepare destination arrays for swscale
          // OpenCV Mat is BGR packed (single buffer)
          uint8_t* dst_data[4] = {bgr.data, nullptr, nullptr, nullptr};
          int dst_linesize[4] = {static_cast<int>(bgr.step[0]), 0, 0, 0};

          // Convert pixel format (e.g., YUV420P -> BGR24)
          sws_scale(sws_ctx,
                    frame->data,
                    frame->linesize,
                    0,
                    height,
                    dst_data,
                    dst_linesize);

          // Invoke callback with decoded BGR frame
          if (on_frame_) {
            on_frame_(bgr);
          }
        }
      }
    }

    // Unref packet to free its internal buffers
    av_packet_unref(packet);
  }

  // Cleanup: release all FFmpeg resources in reverse order of allocation
  av_packet_free(&packet);
  av_frame_free(&frame);
  if (sws_ctx) {
    sws_freeContext(sws_ctx);
  }
  avcodec_free_context(&codec_ctx);
  avformat_close_input(&format_ctx);
  return true;
}

// Signal the receive loop to stop.
// Thread-safe: sets atomic flag checked by Run() loop.
// The loop will exit on the next iteration, triggering cleanup.
void RtpReceiver::Stop() {
  running_ = false;
}

}  // namespace ingest
