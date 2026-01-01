#pragma once

#include <functional>
#include <memory>
#include <string>

#include "api/jsep.h"
#include "api/peer_connection_interface.h"

namespace webrtc_wrap {

class SetDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
 public:
  using SuccessCallback = std::function<void()>;
  using FailureCallback = std::function<void(const std::string&)>;

  static rtc::scoped_refptr<SetDescriptionObserver> Create(SuccessCallback on_success,
                                                          FailureCallback on_failure);

  void OnSuccess() override;
  void OnFailure(webrtc::RTCError error) override;

 private:
  SetDescriptionObserver(SuccessCallback on_success, FailureCallback on_failure);

  SuccessCallback on_success_;
  FailureCallback on_failure_;
};

class CreateDescriptionObserver : public webrtc::CreateSessionDescriptionObserver {
 public:
  using SuccessCallback = std::function<void(webrtc::SessionDescriptionInterface*)>;
  using FailureCallback = std::function<void(const std::string&)>;

  static rtc::scoped_refptr<CreateDescriptionObserver> Create(SuccessCallback on_success,
                                                             FailureCallback on_failure);

  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(webrtc::RTCError error) override;

 private:
  CreateDescriptionObserver(SuccessCallback on_success, FailureCallback on_failure);

  SuccessCallback on_success_;
  FailureCallback on_failure_;
};

class PeerObserver : public webrtc::PeerConnectionObserver {
 public:
  using TrackCallback = std::function<void(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>)>;
  using IceCandidateCallback = std::function<void(const webrtc::IceCandidateInterface*)>;

  PeerObserver(TrackCallback on_track, IceCandidateCallback on_ice);

  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState) override {}
  void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface>) override {}
  void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface>) override {}
  void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface>) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState) override {}
  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState) override {}
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;

 private:
  TrackCallback on_track_;
  IceCandidateCallback on_ice_;
};

}  // namespace webrtc_wrap
