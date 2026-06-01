#include "docker/http/request.hpp"
#include "docker/http/response.hpp"
#include "docker/models/container.hpp"
#include "docker/transport/unix_socket.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

int main() {
  docker::transport::UnixSocket sock;

  docker::http::Request req;
  req.path = "/v1.54/containers/json";
  req.path += "?all=true"; // to see the stopped containers too

  std::string raw = sock.send(req.serialize());
  docker::http::Response res = docker::http::Response::parse(raw);

  // parse the JSON array into a vector of container structs
  auto j = nlohmann::json::parse(res.body);
  auto containers = j.get<std::vector<docker::models::Container>>();

  for (auto &c : containers) {
    std::cout << c.id.substr(0, 12) // docker convention: first 12 chars
              << " " << c.image << " " << c.status << "\n";
  }

  return 0;
}
