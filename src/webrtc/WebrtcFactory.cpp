#include "webrtc/WebrtcFactory.h"

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/peer_connection_interface.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "rtc_base/thread.h"

#include "util/Log.h"

namespace webrtc_wrap {

WebrtcFactory::WebrtcFactory() {
  network_thread_ = rtc::Thread::CreateWithSocketServer();
  worker_thread_ = rtc::Thread::Create();
  signaling_thread_ = rtc::Thread::Create();

  network_thread_->Start();
  worker_thread_->Start();
  signaling_thread_->Start();

  factory_ = webrtc::CreatePeerConnectionFactory(
      network_thread_.get(), worker_thread_.get(), signaling_thread_.get(),
      nullptr, webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      webrtc::CreateBuiltinVideoEncoderFactory(),
      webrtc::CreateBuiltinVideoDecoderFactory(), nullptr, nullptr);

  if (!factory_) {
    LOG_ERROR("Failed to create PeerConnectionFactory");
  }
}

}  // namespace webrtc_wrap
