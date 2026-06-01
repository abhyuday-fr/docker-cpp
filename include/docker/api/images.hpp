#ifndef DOCKER_API_IMAGES_HPP
#define DOCKER_API_IMAGES_HPP

#include "../http/request.hpp"
#include "../http/response.hpp"
#include "../models/image.hpp"
#include "../transport/transport.hpp"
#include "nlohmann/json.hpp"
#include <string>
#include <vector>

namespace docker::api {

class Images {
public:
  explicit Images(docker::transport::ITransport *sock) : sock_(sock) {}

  // POST /images/create?fromImage=alpine&tag=latest
  //  blocks until pull completes
  void pull(const std::string &image, const std::string &tag = "latest") {
    docker::http::Request req;
    req.method = "POST";
    req.path = "/v1.54/images/create" + std::string("?fromimage=") + image +
               "&tag=" + tag;

    sock_->send(req.serialize());
  }

  // GET /images/json
  std::vector<docker::models::Image> list() {
    docker::http::Request req;
    req.path = "/v1.54/images/json";
    auto res = docker::http::Response::parse(sock_->send(req.serialize()));
    return nlohmann::json::parse(res.body)
        .get<std::vector<docker::models::Image>>();
  }

  // DELETE /images/{name}
  void remove(const std::string &name, bool force = false) {
    docker::http::Request req;
    req.method = "DELETE";
    req.path = "/v1.54/images/" + name;
    if (force)
      req.path += "?force=true";
    docker::http::Response::parse(sock_->send(req.serialize()));
  }

private:
  docker::transport::ITransport *sock_;
}; // class Images
} // namespace docker::api

#endif
