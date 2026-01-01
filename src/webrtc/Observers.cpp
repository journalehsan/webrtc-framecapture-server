#include "webrtc/Observers.h"

#include "util/Log.h"

namespace webrtc_wrap {

SetDescriptionObserver::SetDescriptionObserver(SuccessCallback on_success, FailureCallback on_failure)
    : on_success_(std::move(on_success)), on_failure_(std::move(on_failure)) {}

rtc::scoped_refptr<SetDescriptionObserver> SetDescriptionObserver::Create(
    SuccessCallback on_success,
    FailureCallback on_failure) {
  return rtc::scoped_refptr<SetDescriptionObserver>(
      new rtc::RefCountedObject<SetDescriptionObserver>(std::move(on_success), std::move(on_failure)));
}

void SetDescriptionObserver::OnSuccess() {
  if (on_success_) {
    on_success_();
  }
}

void SetDescriptionObserver::OnFailure(webrtc::RTCError error) {
  if (on_failure_) {
    on_failure_(error.message());
  }
}

CreateDescriptionObserver::CreateDescriptionObserver(SuccessCallback on_success,
                                                     FailureCallback on_failure)
    : on_success_(std::move(on_success)), on_failure_(std::move(on_failure)) {}

rtc::scoped_refptr<CreateDescriptionObserver> CreateDescriptionObserver::Create(
    SuccessCallback on_success,
    FailureCallback on_failure) {
  return rtc::scoped_refptr<CreateDescriptionObserver>(
      new rtc::RefCountedObject<CreateDescriptionObserver>(std::move(on_success), std::move(on_failure)));
}

void CreateDescriptionObserver::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  if (on_success_) {
    on_success_(desc);
  }
}

void CreateDescriptionObserver::OnFailure(webrtc::RTCError error) {
  if (on_failure_) {
    on_failure_(error.message());
  }
}

PeerObserver::PeerObserver(TrackCallback on_track, IceCandidateCallback on_ice)
    : on_track_(std::move(on_track)), on_ice_(std::move(on_ice)) {}

void PeerObserver::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  if (on_ice_) {
    on_ice_(candidate);
  }
}

void PeerObserver::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {
  auto receiver = transceiver->receiver();
  if (!receiver) {
    return;
  }
  auto track = receiver->track();
  if (track && on_track_) {
    on_track_(track);
  }
}

}  // namespace webrtc_wrap
