#ifndef DOCKER_EXCEPTION_HPP
#define DOCKER_EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace docker {
class ApiError : public std::runtime_error {
public:
  ApiError(int status_code, const std::string &message)
      : std::runtime_error("Docker API error " + std::to_string(status_code) +
                           ": " + message),
        status_code_(status_code), message_(message) {}

  int status_code() const noexcept { return status_code_; }

  const std::string &message() const noexcept { return message_; }

private:
  int status_code_;
  std::string message_;
}; // class ApiError
} // namespace docker

#endif
