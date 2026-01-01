#pragma once

#include <string>

#include "api/jsep.h"

namespace webrtc_wrap {

class SdpUtils {
 public:
  static webrtc::SessionDescriptionInterface* CreateSessionDescription(
      const std::string& type,
      const std::string& sdp,
      std::string* error);

  static std::string ToString(const webrtc::SessionDescriptionInterface& description);
};

}  // namespace webrtc_wrap
