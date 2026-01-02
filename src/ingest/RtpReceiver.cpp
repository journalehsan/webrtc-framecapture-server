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

std::string AvErrorToString(int err) {
  char buffer[AV_ERROR_MAX_STRING_SIZE];
  av_strerror(err, buffer, sizeof(buffer));
  return buffer;
}

}  // namespace

RtpReceiver::RtpReceiver(std::string url, FrameCallback on_frame)
    : url_(std::move(url)), on_frame_(std::move(on_frame)) {}

bool RtpReceiver::Run() {
  running_ = true;
  avformat_network_init();

  AVFormatContext* format_ctx = nullptr;
  AVDictionary* options = nullptr;

  av_dict_set(&options, "protocol_whitelist", "file,udp,rtp", 0);
  av_dict_set(&options, "analyzeduration", "10000000", 0);
  av_dict_set(&options, "probesize", "5000000", 0);
  int ret = avformat_open_input(&format_ctx, url_.c_str(), nullptr, &options);
  av_dict_free(&options);
  if (ret < 0) {
    LOG_ERROR("Failed to open input: " + AvErrorToString(ret));
    return false;
  }

  ret = avformat_find_stream_info(format_ctx, nullptr);
  if (ret < 0) {
    LOG_ERROR("Failed to find stream info: " + AvErrorToString(ret));
    avformat_close_input(&format_ctx);
    return false;
  }

  int video_stream_index = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (video_stream_index < 0) {
    LOG_ERROR("No video stream found: " + AvErrorToString(video_stream_index));
    avformat_close_input(&format_ctx);
    return false;
  }

  AVStream* video_stream = format_ctx->streams[video_stream_index];
  const AVCodec* codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
  if (!codec) {
    LOG_ERROR("No decoder for codec id: " + std::to_string(video_stream->codecpar->codec_id));
    avformat_close_input(&format_ctx);
    return false;
  }

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

  ret = avcodec_open2(codec_ctx, codec, nullptr);
  if (ret < 0) {
    LOG_ERROR("Failed to open codec: " + AvErrorToString(ret));
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    return false;
  }

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

  cv::Mat bgr;
  int last_width = 0;
  int last_height = 0;

  while (running_) {
    ret = av_read_frame(format_ctx, packet);
    if (ret == AVERROR(EAGAIN)) {
      continue;
    }
    if (ret < 0) {
      LOG_INFO("Stream ended or error: " + AvErrorToString(ret));
      break;
    }

    if (packet->stream_index == video_stream_index) {
      ret = avcodec_send_packet(codec_ctx, packet);
      if (ret < 0) {
        LOG_WARN("Failed to send packet: " + AvErrorToString(ret));
      } else {
        while (ret >= 0) {
          ret = avcodec_receive_frame(codec_ctx, frame);
          if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
          }
          if (ret < 0) {
            LOG_WARN("Failed to decode frame: " + AvErrorToString(ret));
            break;
          }

          const int width = frame->width;
          const int height = frame->height;
          if (width <= 0 || height <= 0) {
            continue;
          }

          if (width != last_width || height != last_height || !sws_ctx) {
            if (sws_ctx) {
              sws_freeContext(sws_ctx);
            }
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
            bgr = cv::Mat(height, width, CV_8UC3);
            last_width = width;
            last_height = height;
          }

          uint8_t* dst_data[4] = {bgr.data, nullptr, nullptr, nullptr};
          int dst_linesize[4] = {static_cast<int>(bgr.step[0]), 0, 0, 0};

          sws_scale(sws_ctx,
                    frame->data,
                    frame->linesize,
                    0,
                    height,
                    dst_data,
                    dst_linesize);

          if (on_frame_) {
            on_frame_(bgr);
          }
        }
      }
    }

    av_packet_unref(packet);
  }

  av_packet_free(&packet);
  av_frame_free(&frame);
  if (sws_ctx) {
    sws_freeContext(sws_ctx);
  }
  avcodec_free_context(&codec_ctx);
  avformat_close_input(&format_ctx);
  return true;
}

void RtpReceiver::Stop() {
  running_ = false;
}

}  // namespace ingest
