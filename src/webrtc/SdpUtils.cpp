#include "webrtc/SdpUtils.h"

#include "api/jsep.h"

namespace webrtc_wrap {

webrtc::SessionDescriptionInterface* SdpUtils::CreateSessionDescription(
    const std::string& type,
    const std::string& sdp,
    std::string* error) {
  webrtc::SdpParseError parse_error;
  webrtc::SessionDescriptionInterface* desc =
      webrtc::CreateSessionDescription(type, sdp, &parse_error).release();
  if (!desc && error) {
    *error = parse_error.description;
  }
  return desc;
}

std::string SdpUtils::ToString(const webrtc::SessionDescriptionInterface& description) {
  std::string sdp;
  description.ToString(&sdp);
  return sdp;
}

}  // namespace webrtc_wrap
