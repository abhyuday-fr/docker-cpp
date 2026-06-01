#ifndef DOCKER_API_VOLUMES_HPP
#define DOCKER_API_VOLUMES_HPP

#include "../http/request.hpp"
#include "../http/response.hpp"
#include "../models/volume.hpp"
#include "../transport/transport.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace docker::api {

class Volumes {
public:
  explicit Volumes(docker::transport::ITransport *sock) : sock_(sock) {}

  // GET /volumes
  // notez; docker wraps the array in {"Volumes":[...], "Warnings":[...]}
  std::vector<docker::models::Volume> list() {
    docker::http::Request req;
    req.path = "/v1.54/volumes";
    auto res = docker::http::Response::parse(sock_->send(req.serialize()));

    auto j = nlohmann::json::parse(res.body);

    return j.at("Volumes").get<std::vector<docker::models::Volume>>();
  }

  // POST /volumes/create
  docker::models::Volume create(const std::string &name,
                                const std::string &driver = "local") {
    nlohmann::json body = {{"Name", name}, {"Driver", driver}};

    docker::http::Request req;
    req.method = "POST";
    req.path = "/v1.54/volumes/create";
    req.body = body.dump();

    auto res = docker::http::Response::parse(sock_->send(req.serialize()));
    return nlohmann::json::parse(res.body).get<docker::models::Volume>();
  }

  // DELETE /volumes/{name}
  void remove(const std::string &name) {
    docker::http::Request req;
    req.method = "DELETE";
    req.path = "/v1.54/volumes/" + name;
    docker::http::Response::parse(sock_->send(req.serialize()));
    // docker return 409 is volume is in use, Response::parse throws it
  }

private:
  docker::transport::ITransport *sock_;
}; // class Volumes
} // namespace docker::api

#endif
