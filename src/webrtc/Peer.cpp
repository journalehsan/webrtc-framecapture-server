#include "webrtc/Peer.h"

#include <chrono>
#include <future>

#include "api/jsep.h"
#include "api/rtp_transceiver_interface.h"
#include "rtc_base/ref_counted_object.h"

#include "util/Log.h"
#include "webrtc/Observers.h"
#include "webrtc/SdpUtils.h"
#include "webrtc/VideoFrameSink.h"

namespace webrtc_wrap {

Peer::Peer(rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory,
           std::shared_ptr<media::FrameWriter> writer)
    : factory_(std::move(factory)), writer_(std::move(writer)) {
  sink_ = std::make_unique<VideoFrameSink>(writer_);
}

bool Peer::Initialize() {
  if (!factory_) {
    return false;
  }
  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

  observer_ = std::make_unique<PeerObserver>(
      [this](rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track) {
        AttachVideoTrack(track);
      },
      [](const webrtc::IceCandidateInterface* candidate) {
        if (!candidate) {
          return;
        }
        std::string sdp;
        candidate->ToString(&sdp);
        LOG_INFO("ICE candidate gathered: " + sdp);
      });

  connection_ = factory_->CreatePeerConnection(config, nullptr, nullptr, observer_.get());
  if (!connection_) {
    LOG_ERROR("Failed to create PeerConnection");
    return false;
  }
  return true;
}

std::string Peer::HandleOffer(const std::string& sdp) {
  if (!connection_) {
    return {};
  }

  std::promise<std::string> answer_promise;
  auto answer_future = answer_promise.get_future();

  std::string error;
  std::unique_ptr<webrtc::SessionDescriptionInterface> offer(
      SdpUtils::CreateSessionDescription("offer", sdp, &error));
  if (!offer) {
    LOG_ERROR("Failed to parse offer: " + error);
    return {};
  }

  connection_->SetRemoteDescription(
      SetDescriptionObserver::Create(
          [this, &answer_promise]() {
            webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
            connection_->CreateAnswer(
                CreateDescriptionObserver::Create(
                    [this, &answer_promise](webrtc::SessionDescriptionInterface* desc) {
                      connection_->SetLocalDescription(
                          SetDescriptionObserver::Create(
                              [desc, &answer_promise]() {
                                std::string sdp_answer = SdpUtils::ToString(*desc);
                                answer_promise.set_value(sdp_answer);
                              },
                              [&answer_promise](const std::string& error) {
                                LOG_ERROR("SetLocalDescription failed: " + error);
                                answer_promise.set_value("");
                              }),
                          desc);
                    },
                    [&answer_promise](const std::string& error) {
                      LOG_ERROR("CreateAnswer failed: " + error);
                      answer_promise.set_value("");
                    }),
                options);
          },
          [&answer_promise](const std::string& error) {
            LOG_ERROR("SetRemoteDescription failed: " + error);
            answer_promise.set_value("");
          }),
      offer.release());

  auto status = answer_future.wait_for(std::chrono::seconds(5));
  if (status != std::future_status::ready) {
    LOG_ERROR("Timed out waiting for SDP answer");
    return {};
  }
  return answer_future.get();
}

bool Peer::AddIceCandidate(const std::string& sdp_mid, int sdp_mline_index,
                           const std::string& candidate) {
  webrtc::SdpParseError error;
  std::unique_ptr<webrtc::IceCandidateInterface> ice(
      webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, candidate, &error));
  if (!ice) {
    LOG_ERROR("Failed to parse ICE candidate: " + error.description);
    return false;
  }
  if (!connection_->AddIceCandidate(ice.get())) {
    LOG_WARN("AddIceCandidate returned false");
    return false;
  }
  return true;
}

void Peer::AttachVideoTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track) {
  if (!track || track->kind() != webrtc::MediaStreamTrackInterface::kVideoKind) {
    return;
  }
  auto video_track = static_cast<webrtc::VideoTrackInterface*>(track.get());
  video_track->AddOrUpdateSink(sink_.get(), rtc::VideoSinkWants());
  LOG_INFO("Video track attached");
}

}  // namespace webrtc_wrap
