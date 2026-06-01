#ifndef DOCKER_API_CONTAINERS_HPP
#define DOCKER_API_CONTAINERS_HPP

#include "../http/request.hpp"
#include "../http/response.hpp"
#include "../models/container.hpp"
#include "../models/log_line.hpp"
#include "../transport/thread_pool.hpp"
#include "../transport/transport.hpp"
#include <exception>
#include <functional>
#include <future>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace docker::api {
class Containers {
public:
  explicit Containers(docker::transport::ITransport *sock,
                      std::size_t thread_count = 8)
      : sock_(sock), pool_(thread_count, sock_->make_factory()) {}

  // GET /containers/json
  std::vector<docker::models::Container> list(bool all = false) {
    docker::http::Request req;
    req.path = "/v1.54/containers/json";
    if (all)
      req.path += "?all=true";

    auto res = docker::http::Response::parse(sock_->send(req.serialize()));
    return nlohmann::json::parse(res.body)
        .get<std::vector<docker::models::Container>>();
  }

  // POST /containers/{id}/start
  void start(const std::string &id) {
    docker::http::Request req;
    req.method = "POST";
    req.path = "/v1.54/containers/" + id + "/start";
    docker::http::Response::parse(sock_->send(req.serialize()));
  }

  // POST /containers/{id}/stop
  void stop(const std::string &id, int timeout = 10) {
    docker::http::Request req;
    req.method = "POST";
    req.path =
        "/v1.54/containers/" + id + "/stop" + "?t=" + std::to_string(timeout);
    docker::http::Response::parse(sock_->send(req.serialize()));
  }

  // DELETE /containers/{id}/
  void remove(const std::string &id, bool force = false) {
    docker::http::Request req;
    req.method = "DELETE";
    req.path = "/v1.54/containers/" + id;
    if (force)
      req.path += "?force=true";
    docker::http::Response::parse(sock_->send(req.serialize()));
  }

  // POST /containers/create (minimal version)
  std::string create(const std::string &image, const std::string &name = "") {
    nlohmann::json body = {{"Image", image}};

    docker::http::Request req;
    req.method = "POST";
    req.path = "/v1.54/containers/create";
    if (!name.empty())
      req.path += "?name=" + name;
    req.body = body.dump();

    auto res = docker::http::Response::parse(sock_->send(req.serialize()));

    return nlohmann::json::parse(res.body).at("Id").get<std::string>();
  }

  // POST /containers/create + start in one call
  std::string run(const std::string &image,
                  const std::vector<std::string> &cmd = {}) {
    nlohmann::json body = {{"Image", image}};
    if (!cmd.empty())
      body["Cmd"] = cmd;

    docker::http::Request req;
    req.method = "POST";
    req.path = "/v1.54/containers/create";
    req.body = body.dump();

    auto res = docker::http::Response::parse(sock_->send(req.serialize()));
    std::string id =
        nlohmann::json::parse(res.body).at("Id").get<std::string>();

    start(id);
    return id;
  }

  // stop multiple containers in parallel via thread pool
  void stop_all(const std::vector<std::string> &ids, int timeout = 10) {
    std::vector<std::future<void>> futures;
    for (auto &id : ids) {
      futures.push_back(
          pool_.submit([id, timeout](docker::transport::ITransport &sock) {
            docker::http::Request req;
            req.method = "POST";
            req.path = "/v1.54/containers/" + id +
                       "/stop?t=" + std::to_string(timeout);
            docker::http::Response::parse(sock.send(req.serialize()));
          }));
    }
    wait_all(futures);
  }

  // remove multiple containers in parallel via thread pool
  void remove_all(const std::vector<std::string> &ids, bool force = false) {
    std::vector<std::future<void>> futures;
    for (auto &id : ids) {
      futures.push_back(
          pool_.submit([id, force](docker::transport::ITransport &sock) {
            docker::http::Request req;
            req.method = "DELETE";
            req.path = "/v1.54/containers/" + id;
            if (force)
              req.path += "?force=true";
            docker::http::Response::parse(sock.send(req.serialize()));
          }));
    }
    wait_all(futures);
  }

  // GET /containers/{id}/logs
  void logs(const std::string &id,
            const std::function<void(int, const std::string &)> &callback,
            bool follow = false, bool stdout_ = true, bool stderr_ = true,
            int tail = 100) {
    docker::http::Request req;
    req.path = "/v1.54/containers/" + id + "/logs" +
               "?stdout=" + (stdout_ ? "true" : "false") +
               "&stderr=" + (stderr_ ? "true" : "false") +
               "&follow=" + (follow ? "true" : "false") +
               "&tail=" + (tail > 0 ? std::to_string(tail) : "all");

    // for follow=true this blocks until the container exists
    std::string raw = sock_->send(req.serialize());
    docker::http::Response res = docker::http::Response::parse(raw);

    // parse the multiplexed og frames
    auto lines = docker::models::parse_log_frames(res.body);
    for (auto &line : lines) {
      callback(line.stream, line.text);
    }
  }

private:
  docker::transport::ITransport *sock_;
  docker::transport::ThreadPool pool_;

  // waits for all futures, collects all exceptions, rethrows the first.
  // never returns the first, every future is consumed before rethrowing
  static void wait_all(std::vector<std::future<void>> &futures) {
    std::vector<std::exception_ptr> errors;
    for (auto &f : futures) {
      try {
        f.get();
      } catch (...) {
        errors.push_back(std::current_exception());
      }
    }
    if (!errors.empty())
      std::rethrow_exception(errors[0]);
  }
}; // class Containers
} // namespace docker::api

#endif
