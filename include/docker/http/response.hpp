#ifndef DOCKER_HTTP_RESPONSE_HPP
#define DOCKER_HTTP_RESPONSE_HPP

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>

#include "../exception.hpp"

namespace docker::http {

struct Response {
  int status_code;
  std::map<std::string, std::string> headers;
  std::string body;

  // parse a raw HTTP response string into a Response struct
  static Response parse(const std::string &raw) {
    Response res;

    // splitting headers from body on the blank line
    auto sep = raw.find("\r\n\r\n");
    if (sep == std::string::npos) {
      throw std::runtime_error(
          "Malformed HTTP response: no header/body separator");
    }

    std::string header_section = raw.substr(0, sep);
    res.body = raw.substr(sep + 4); // skip \r\n\r\n

    // parsing the status line (HTTP/1.1 200 OK)
    auto first_line_end = header_section.find("\r\n");
    std::string status_line = header_section.substr(0, first_line_end);
    // status code is the second token
    auto first_space = status_line.find(' ');
    auto second_space = status_line.find(' ', first_space + 1);
    res.status_code = std::stoi(
        status_line.substr(first_space + 1, second_space - first_space - 1));

    // parsing the remaining headers (Key, value)
    std::string remaining = header_section.substr(first_line_end + 2);
    while (!remaining.empty()) {
      auto line_end = remaining.find("\r\n");
      std::string line = (line_end == std::string::npos)
                             ? remaining
                             : remaining.substr(0, line_end);

      auto colon = line.find(':');
      if (colon != std::string::npos) {
        std::string key = line.substr(0, colon);
        std::string value = (colon + 2 <= line.size() ? line.substr(colon + 2)
                                                      : " "); // skip ": "
        res.headers[key] = value;
      }

      if (line_end == std::string::npos)
        break;
      remaining = remaining.substr(line_end + 2);
    }

    auto it = res.headers.find("Transfer-Encoding");
    if (it != res.headers.end() && it->second == "chunked")
      res.body = decode_chunked(res.body);

    // throw on HTTP errors so callers don't have to check the status codes
    if (res.status_code >= 400) {
      throw docker::ApiError(res.status_code, res.body);
    }

    return res;
  }

  static std::string decode_chunked(const std::string &body) {
    std::string result;
    std::size_t pos = 0;

    while (pos < body.size()) {
      // find end of chunk size line
      auto line_end = body.find("\r\n", pos);
      if (line_end == std::string::npos)
        break;

      // parse hex chunk size
      std::string size_str = body.substr(pos, line_end - pos);

      // strip chunk extensions if any present
      auto semi = size_str.find(';');
      if (semi != std::string::npos)
        size_str = size_str.substr(0, semi);

      std::size_t chunk_size = std::stoul(size_str, nullptr, 16);
      if (chunk_size == 0)
        break;

      pos = line_end + 2; // skip past \r\n

      result.append(body, pos, chunk_size);
      pos += chunk_size + 2;
    }

    return result;
  }

}; // struct response
} // namespace docker::http

#endif
