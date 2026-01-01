#pragma once

#include <memory>

#include "api/peer_connection_interface.h"

namespace webrtc_wrap {

class WebrtcFactory {
 public:
  WebrtcFactory();

  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory() const { return factory_; }

 private:
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
  std::unique_ptr<rtc::Thread> network_thread_;
  std::unique_ptr<rtc::Thread> worker_thread_;
  std::unique_ptr<rtc::Thread> signaling_thread_;
};

}  // namespace webrtc_wrap
