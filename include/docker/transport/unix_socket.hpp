#ifndef DOCKER_TRANSPORT_UNIX_SOCKET_HPP
#define DOCKER_TRANSPORT_UNIX_SOCKET_HPP

#include "transport.hpp"
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace docker::transport {

class UnixSocket : public ITransport {
public:
#ifdef _WIN32
  explicit UnixUnixSocket(std::string path = "\\\\.\\pipe\\docker_engine");
#else
  explicit UnixSocket(std::string path = "/var/run/docker.sock")
#endif
      : path_(std::move(path)),
#ifdef _WIN32
      , pipe_(INVALID_HANDLE_VALUE)
#else
        fd_(-1)
#endif
        {
      }

      ~UnixSocket() override { disconnect(); }

      UnixSocket(const UnixSocket &) = delete;
      UnixSocket &operator=(const UnixSocket &) = delete;

      UnixSocket(UnixSocket &&other) noexcept
          : path_(std::move(other.path_))
#ifdef _WIN32
            ,
            pipe_(other.pipe_)
#else
        ,
        fd_(other.fd_)
#endif
      {
#ifdef _WIN32
        other.pipe_ = INVALID_HANDLE_VALUE;
#else
    other.fd_ = -1;
#endif
      }

      UnixSocket &operator=(UnixSocket &&other) noexcept {
        if (this != &other) {
          disconnect();
          path_ = std::move(other.path_);
#ifdef _WIN32
          pipe_ = other.pipe_;
          other.pipe_ = INVALID HANDLE VALUE;
#else
      fd_ = other.fd_;
      other.fd_ = -1;
#endif
        }
        return *this;
      }

      // path accessor used by make_factory and ThreadPool
      const std::string &path() const { return path_; }

      // ITransport each worker thread gets its own UnixSocket via this factory
      ITransport::Factory make_factory() const override {
        return [path = path_] { return std::make_unique<UnixSocket>(path); };
      }

      // ITransport connect, send, recv, disconnect
      std::string send(const std::string &raw_http) override {
        connect_socket();
#ifdef _WIN32
        const char *data = raw_http.c_str();
        DWORD remaining = static_cast<DWORD>(raw_http.size());
        while (remaining > 0) {
          DWORD written = 0;
          if (!WriteFile(pipe_, data, remaining, &written, nullptr))
            throw std::runtime_error("WriteFile() failed: " +
                                     std::to_string(GetLastError()));
          data += written;
          remaining -= written;
        }

        std::string response;
        char buf[4096];
        DWORD n = 0;
        while (ReadFile(pipe_, buf, sizeof(buf) - 1, &n, nullptr) && n > 0) {
          buf[n] = '\0';
          response.append(buf, n);
        }
#else
    const char *data = raw_http.c_str();
    std::size_t remaining = raw_http.size();
    while (remaining > 0) {
      ssize_t sent = ::send(fd_, data, remaining, 0);
      if (sent < 0)
        throw std::runtime_error("send() failed: " +
                                 std::string(strerror(errno)));
      data += sent;
      remaining -= static_cast<std::size_t>(sent);
    }

    std::string response;
    char buf[4096];
    ssize_t n;
    while ((n = recv(fd_, buf, sizeof(buf) - 1, 0)) > 0) {
      buf[n] = '\0';
      response.append(buf, static_cast<std::size_t>(n));
    }
    if (n < 0)
      throw std::runtime_error("recv() failed: " +
                               std::string(strerror(errno)));
#endif
        disconnect();
        return response;
      }

    private:
      std::string path_;
#ifdef _WIN32
      HANDLE pipe_;
#else
  int fd_;
#endif

      void connect_socket() {
        disconnect();

#ifdef _WIN32
        pipe_ = CreateFileA(path_.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                            nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe_ == INVALID_HANDLE_VALUE)
          throw std::runtime_error("CreateFile() failed: " +
                                   std::to_string(GetLastError()));

#else
    fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd_ < 0)
      throw std::runtime_error("socket() error: " +
                               std::string(strerror(errno)));

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path_.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
      ::close(fd_);
      fd_ = -1;
      throw std::runtime_error("connect() error: " +
                               std::string(strerror(errno)));
    }
#endif
      }

      void disconnect() {
#ifdef _WIN32
        if (pipe != INVALID_HANDLE_VALUE) {
          CloseHandle(pipe_);
          pipe_ = INVALID_HANDLE_VALUE;
        }
#else
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
#endif
      }
}; // class UnixSocket

} // namespace docker::transport

#endif
