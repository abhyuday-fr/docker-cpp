#ifndef DOCKER_DOCKER_HPP
#define DOCKER_DOCKER_HPP

#include "api/containers.hpp"
#include "api/images.hpp"
#include "api/networks.hpp"
#include "api/volumes.hpp"
#include "transport/unix_socket.hpp"
#include <string>

namespace docker {

class Client {
private:
  transport::UnixSocket sock_; // declared first, everything else borrows socket

public:
  explicit Client(const std::string &socket_path = "/var/run/docker.sock",
                  std::size_t thread_count = 8)
      : sock_(socket_path), containers(&sock_, thread_count), images(&sock_),
        networks(&sock_), volumes(&sock_) {}

  // no copy, owns a socket and a thread pool and neither can be shared
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;

  // public API surface
  api::Containers containers;
  api::Images images;
  api::Networks networks;
  api::Volumes volumes;
}; // class Client
} // namespace docker

#endif
