#include "docker/http/request.hpp"
#include "docker/http/response.hpp"
#include "docker/transport/unix_socket.hpp"
#include <iostream>

int main() {
  docker::transport::UnixSocket sock;
  docker::http::Request req;
  req.method = "GET";
  req.path = "/v1.54/version";

  std::string raw = sock.send(req.serialize());
  docker::http::Response res = docker::http::Response::parse(raw);

  std::cout << "Status: " << res.status_code << "\n";
  std::cout << "Body: " << res.body << "\n";
}
