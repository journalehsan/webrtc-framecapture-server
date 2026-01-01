#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

#include "api/peer_connection_interface.h"

#include "webrtc/Observers.h"

#include "media/FrameWriter.h"

namespace webrtc_wrap {

class Peer {
 public:
  Peer(rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory,
       std::shared_ptr<media::FrameWriter> writer);

  bool Initialize();
  std::string HandleOffer(const std::string& sdp);
  bool AddIceCandidate(const std::string& sdp_mid, int sdp_mline_index, const std::string& candidate);

 private:
  void AttachVideoTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track);

  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> connection_;
  std::shared_ptr<media::FrameWriter> writer_;
  std::unique_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink_;
  std::unique_ptr<PeerObserver> observer_;
};

}  // namespace webrtc_wrap
