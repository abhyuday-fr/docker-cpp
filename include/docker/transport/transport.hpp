#ifndef DOCKER_TRANSPORT_TRANSPORT_HPP
#define DOCKER_TRANSPORT_TRANSPORT_HPP

#include <functional>
#include <memory>
#include <string>

namespace docker::transport {
class ITransport {
public:
  virtual ~ITransport() = default;

  virtual std::string send(const std::string &raw_http) = 0;

  virtual std::function<std::unique_ptr<ITransport>()> make_factory() const = 0;

  // used by ThreadPool to create one transport per worker thread
  using Factory = std::function<std::unique_ptr<ITransport>()>;
}; // class ITransport
} // namespace docker::transport

#endif
