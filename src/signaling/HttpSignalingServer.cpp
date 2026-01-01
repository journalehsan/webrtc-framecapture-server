#include "signaling/HttpSignalingServer.h"

#include <nlohmann/json.hpp>

#include "util/Log.h"

namespace signaling {

HttpSignalingServer::HttpSignalingServer(int port, OfferHandler on_offer,
                                         CandidateHandler on_candidate)
    : port_(port), on_offer_(std::move(on_offer)), on_candidate_(std::move(on_candidate)) {
  server_ = std::make_unique<httplib::Server>();
  RegisterRoutes();
}

HttpSignalingServer::~HttpSignalingServer() { Stop(); }

void HttpSignalingServer::RegisterRoutes() {
  server_->Post("/offer", [this](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json body = nlohmann::json::parse(req.body, nullptr, false);
    if (body.is_discarded() || !body.contains("sdp")) {
      res.status = 400;
      res.set_content("Invalid offer", "text/plain");
      return;
    }
    if (!on_offer_) {
      res.status = 500;
      res.set_content("Offer handler not set", "text/plain");
      return;
    }
    std::string answer = on_offer_(body["sdp"].get<std::string>());
    if (answer.empty()) {
      res.status = 500;
      res.set_content("Failed to create answer", "text/plain");
      return;
    }
    nlohmann::json response{{"type", "answer"}, {"sdp", answer}};
    res.set_content(response.dump(), "application/json");
  });

  server_->Post("/candidate", [this](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json body = nlohmann::json::parse(req.body, nullptr, false);
    if (body.is_discarded() || !body.contains("candidate")) {
      res.status = 400;
      res.set_content("Invalid candidate", "text/plain");
      return;
    }
    if (!on_candidate_) {
      res.status = 500;
      res.set_content("Candidate handler not set", "text/plain");
      return;
    }
    std::string sdp_mid = body.value("sdpMid", "0");
    int sdp_mline = body.value("sdpMLineIndex", 0);
    std::string candidate = body["candidate"].get<std::string>();
    on_candidate_(sdp_mid, sdp_mline, candidate);
    res.status = 200;
  });
}

void HttpSignalingServer::Start() {
  if (server_thread_.joinable()) {
    return;
  }
  server_thread_ = std::thread([this]() {
    LOG_INFO("HTTP signaling server listening on port " + std::to_string(port_));
    server_->listen("0.0.0.0", port_);
  });
}

void HttpSignalingServer::Stop() {
  if (server_) {
    server_->stop();
  }
  if (server_thread_.joinable()) {
    server_thread_.join();
  }
}

}  // namespace signaling
