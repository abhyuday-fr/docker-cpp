#ifndef DOCKER_TRANSPORT_MOCK_TRANSPORT_HPP
#define DOCKER_TRANSPORT_MOCK_TRANSPORT_HPP

#include "transport.hpp"
#include <functional>
#include <memory>
#include <string>

namespace docker::transport {
// a transport that returns hardcoded response, no socket, no docker needed
// used for unit testing the HTTP/JSON/API layers in isolation
class MockTransport : public ITransport {
public:
  // simplest form -> always return the same response
  explicit MockTransport(std::string response)
      : response_(std::move(response)) {}

  // advanced form -> lets you inspect the request and vary the request
  explicit MockTransport(
      std::function<std::string(const std::string &)> handler)
      : handler_(std::move(handler)) {}

  std::string send(const std::string &raw_http) override {
    if (handler_)
      return handler_(raw_http);
    return response_;
  }

  ITransport::Factory make_factory() const override {
    if (handler_) {
      auto h = handler_;
      return [h] { return std::make_unique<MockTransport>(h); };
    }
    auto r = response_;
    return [r] { return std::make_unique<MockTransport>(r); };
  }

private:
  std::string response_;
  std::function<std::string(const std::string &)> handler_;
}; // class MockTransport
} // namespace docker::transport

#endif
