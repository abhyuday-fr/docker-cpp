#ifndef DOCKER_API_NETWORKS_HPP
#define DOCKER_API_NETWORKS_HPP

#include "../http/request.hpp"
#include "../http/response.hpp"
#include "../models/network.hpp"
#include "../transport/transport.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace docker::api {
class Networks {
public:
  explicit Networks(docker::transport::ITransport *sock) : sock_(sock) {}

  // GET /networks
  std::vector<docker::models::Network> list() {
    docker::http::Request req;
    req.path = "/v1.54/networks";
    auto res = http::Response::parse(sock_->send(req.serialize()));
    return nlohmann::json::parse(res.body)
        .get<std::vector<docker::models::Network>>();
  }

  // POST /networks/create, returns the network ID
  std::string create(const std::string &name,
                     const std::string &driver = "bridge",
                     bool internal = false) {
    nlohmann::json body = {
        {"Name", name}, {"Driver", driver}, {"Internal", internal}};

    docker::http::Request req;
    req.method = "POST";
    req.path = "/v1.54/networks/create";
    req.body = body.dump();

    auto res = docker::http::Response::parse(sock_->send(req.serialize()));
    return nlohmann::json::parse(res.body).at("Id").get<std::string>();
  }

  // DELETE /networks/{id}
  void remove(const std::string &id) {
    docker::http::Request req;
    req.method = "DELETE";
    req.path = "/v1.54/networks/" + id;
    docker::http::Response::parse(sock_->send(req.serialize()));
  }

  // POST /networks/{id}/connect
  void connect(const std::string &network_id, const std::string &container_id) {
    nlohmann::json body = {{"Container", container_id}};

    docker::http::Request req;
    req.method = "POST";
    req.path = "/v1.54/networks/" + network_id + "/connect";
    req.body = body.dump();

    docker::http::Response::parse(sock_->send(req.serialize()));
  }

  // POST /networks/{id}/disconnect
  void disconnect(const std::string &network_id,
                  const std::string &container_id, bool force = false) {
    nlohmann::json body = {{"Container", container_id}, {"Force", force}};

    docker::http::Request req;
    req.method = "POST";
    req.path = "/v1.54/networks/" + network_id + "/disconnect";
    req.body = body.dump();

    docker::http::Response::parse(sock_->send(req.serialize()));
  }

private:
  docker::transport::ITransport *sock_;

}; // Class Networks
} // namespace docker::api

#endif
