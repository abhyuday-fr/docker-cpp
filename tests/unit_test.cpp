#include "docker/api/containers.hpp"
#include "docker/api/images.hpp"
#include "docker/api/networks.hpp"
#include "docker/api/volumes.hpp"
#include "docker/transport/mock_transport.hpp"
#include <cassert>
#include <iostream>

using namespace docker::transport;

// helper — wraps a JSON body in a minimal HTTP 200 response
static std::string http_ok(const std::string &body) {
  return "HTTP/1.1 200 OK\r\n"
         "Content-Type: application/json\r\n"
         "Content-Length: " +
         std::to_string(body.size()) +
         "\r\n"
         "\r\n" +
         body;
}

static std::string http_err(int code, const std::string &msg) {
  std::string body = "{\"message\":\"" + msg + "\"}";
  std::string status = code == 404 ? "404 Not Found" : std::to_string(code);
  return "HTTP/1.1 " + status +
         "\r\n"
         "Content-Type: application/json\r\n"
         "Content-Length: " +
         std::to_string(body.size()) +
         "\r\n"
         "\r\n" +
         body;
}

// ── test: containers.list() parses response correctly ─────────────────────

void test_containers_list_empty() {
  MockTransport mock(http_ok("[]"));
  docker::api::Containers containers(&mock);

  auto list = containers.list(true);
  assert(list.empty());
  std::cout << "PASS  containers.list() empty array\n";
}

void test_containers_list_one() {
  std::string body = R"([{
        "Id": "abc123def456abc123def456",
        "Names": ["/my-container"],
        "Image": "alpine",
        "Command": "echo hello",
        "Created": 1700000000,
        "State": "running",
        "Status": "Up 2 hours",
        "Ports": [],
        "Labels": {}
    }])";

  MockTransport mock(http_ok(body));
  docker::api::Containers containers(&mock);

  auto list = containers.list();
  assert(list.size() == 1);
  assert(list[0].image == "alpine");
  assert(list[0].state == "running");
  assert(list[0].names[0] == "/my-container");
  std::cout << "PASS  containers.list() one container\n";
}

// ── test: ApiError carries status code ────────────────────────────────────

void test_api_error_status_code() {
  MockTransport mock(http_err(404, "No such container: bad-id"));
  docker::api::Containers containers(&mock);

  try {
    containers.start("bad-id");
    assert(false && "should have thrown");
  } catch (const docker::ApiError &e) {
    assert(e.status_code() == 404);
    std::cout << "PASS  ApiError carries status code 404\n";
  }
}

void test_api_error_is_runtime_error() {
  MockTransport mock(http_err(500, "internal error"));
  docker::api::Containers containers(&mock);

  try {
    containers.list();
    assert(false && "should have thrown");
  } catch (const std::runtime_error &e) {
    // ApiError IS-A runtime_error — must be catchable here too
    std::cout << "PASS  ApiError caught as std::runtime_error\n";
  }
}

// ── test: images.list() parses correctly ──────────────────────────────────

void test_images_list() {
  std::string body = R"([{
        "Id": "sha256:abc123",
        "RepoTags": ["alpine:latest"],
        "Created": 1700000000,
        "Size": 7338965
    }])";

  MockTransport mock(http_ok(body));
  docker::api::Images images(&mock);

  auto list = images.list();
  assert(list.size() == 1);
  assert(list[0].repo_tags[0] == "alpine:latest");
  std::cout << "PASS  images.list() one image\n";
}

// ── test: volumes.list() handles wrapped response ─────────────────────────

void test_volumes_list_wrapped() {
  std::string body = R"({
        "Volumes": [{
            "Name": "my-vol",
            "Driver": "local",
            "Mountpoint": "/var/lib/docker/volumes/my-vol/_data",
            "Scope": "local"
        }],
        "Warnings": []
    })";

  MockTransport mock(http_ok(body));
  docker::api::Volumes volumes(&mock);

  auto list = volumes.list();
  assert(list.size() == 1);
  assert(list[0].name == "my-vol");
  assert(list[0].driver == "local");
  std::cout << "PASS  volumes.list() unwraps Volumes key\n";
}

// ── test: mock can inspect the request ────────────────────────────────────

void test_mock_inspects_request() {
  std::string captured_request;

  MockTransport mock([&captured_request](const std::string &req) {
    captured_request = req;
    return "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\n[]";
  });

  docker::api::Containers containers(&mock);
  containers.list(true);

  // verify the request contained the right path and query
  assert(captured_request.find("GET /v1.54/containers/json") !=
         std::string::npos);
  assert(captured_request.find("all=true") != std::string::npos);
  std::cout << "PASS  mock captures request content\n";
}

int main() {
  std::cout << "=== Unit Tests (no Docker daemon required) ===\n\n";

  test_containers_list_empty();
  test_containers_list_one();
  test_api_error_status_code();
  test_api_error_is_runtime_error();
  test_images_list();
  test_volumes_list_wrapped();
  test_mock_inspects_request();

  std::cout << "\nAll tests passed.\n";
  return 0;
}
