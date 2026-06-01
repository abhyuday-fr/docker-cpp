#ifndef DOCKER_HTTP_REQUEST_HPP
#define DOCKER_HTTP_REQUEST_HPP

#include <map>
#include <sstream>
#include <string>

namespace docker::http {
struct Request {
  std::string method = "GET";
  std::string path;
  std::string body;
  std::map<std::string, std::string> headers;

  std::string serialize() const {
    std::ostringstream out;
    out << method << " " << path << " HTTP/1.1\r\n";
    out << "Host: localhost\r\n";

    for (auto &[k, v] : headers) {
      out << k << ": " << v << "\r\n";
    }

    if (!body.empty()) {
      out << "Content-Type: application/json\r\n";
      out << "Content-Length: " << body.size() << "\r\n";
    }

    out << "Connection: close\r\n\r\n";

    if (!body.empty()) {
      out << body;
    }

    return out.str();
  }
}; // struct Request
} // namespace docker::http

#endif
