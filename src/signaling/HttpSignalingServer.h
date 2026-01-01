#pragma once

#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "httplib.h"

namespace signaling {

class HttpSignalingServer {
 public:
  using OfferHandler = std::function<std::string(const std::string&)>;
  using CandidateHandler = std::function<void(const std::string&, int, const std::string&)>;

  HttpSignalingServer(int port, OfferHandler on_offer, CandidateHandler on_candidate);
  ~HttpSignalingServer();

  void Start();
  void Stop();

 private:
  void RegisterRoutes();

  int port_;
  OfferHandler on_offer_;
  CandidateHandler on_candidate_;
  std::unique_ptr<httplib::Server> server_;
  std::thread server_thread_;
};

}  // namespace signaling
