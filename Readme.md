
# docker-cpp

A header-only C++17 SDK for the Docker Engine API. Talk to Docker from C++ without
shell commands, subprocess hacks, or raw HTTP strings. Just clean, typed API calls
over the Docker Unix socket (Linux/macOS) or named pipe (Windows).

```cpp
#include <docker/docker.hpp>

int main() {
    docker::Client client;

    client.images.pull("alpine");

    std::string id = client.containers.run("alpine", {"echo", "hello from C++"});
    client.containers.stop(id, 5);
    client.containers.remove(id, true);
}
```

## Features

- **Header-only** : drop into any CMake project, no compilation step
- **Full container lifecycle** : create, run, start, stop, remove
- **Images** : pull, list, remove
- **Networks** : create, list, connect, disconnect, remove
- **Volumes** : create, list, remove
- **Bulk operations** : stop and remove multiple containers in parallel via a thread pool
- **Log streaming** : fetch stdout/stderr with per-stream callbacks
- **Typed error handling** : `docker::ApiError` with accessible HTTP status code
- **Testable** : `ITransport` abstraction and `MockTransport` for unit tests without a live daemon
- **RAII throughout** : sockets, threads, and all resources cleaned up automatically
- **Cross-platform** : Unix socket on Linux/macOS, named pipe on Windows

## Requirements

- C++17 compiler (GCC 8+, Clang 7+, MSVC 19.14+)
- CMake 3.15+
- [nlohmann/json](https://github.com/nlohmann/json) v3.11+ (fetched automatically via CMake)
- Docker daemon running and accessible

## Installation

### Via CMake FetchContent (recommended)

```cmake
include(FetchContent)
FetchContent_Declare(
    docker-cpp
    GIT_REPOSITORY https://github.com/yourusername/docker-cpp
    GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(docker-cpp)

target_link_libraries(your_target PRIVATE docker-cpp::docker-cpp)
```

No other steps needed — nlohmann/json is fetched automatically.

### Via cmake --install

```bash
git clone https://github.com/yourusername/docker-cpp
cd docker-cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

Then in your CMakeLists.txt:

```cmake
find_package(docker-cpp REQUIRED)
target_link_libraries(your_target PRIVATE docker-cpp::docker-cpp)
```

## Quick start

```cpp
#include <docker/docker.hpp>
#include <docker/exception.hpp>
#include <iostream>

int main() {
    docker::Client client; // connects to /var/run/docker.sock by default

    // pull an image
    client.images.pull("alpine");

    // run a container and get its ID
    std::string id = client.containers.run("alpine", {"echo", "hello"});

    // fetch its logs
    client.containers.logs(id, [](int stream, const std::string& line) {
        std::cout << (stream == 1 ? "[stdout] " : "[stderr] ") << line;
    });

    // cleanup
    client.containers.stop(id, 5);
    client.containers.remove(id, true);
}
```

## API reference

### Client

```cpp
// default socket path, 8 worker threads for bulk operations
docker::Client client;

// custom socket path and thread count
docker::Client client("/var/run/docker.sock", 16);
```

`docker::Client` is non-copyable. It owns the socket and thread pool — create one
per application and share it by reference or pointer.

---

### Containers

```cpp
// list running containers
auto containers = client.containers.list();

// list all containers including stopped ones
auto all = client.containers.list(true);

for (auto& c : all)
    std::cout << c.id.substr(0, 12) << "  " << c.image << "  " << c.status << "\n";

// create a container without starting it
std::string id = client.containers.create("alpine");
std::string id = client.containers.create("alpine", "my-container-name");

// start / stop / remove
client.containers.start(id);
client.containers.stop(id, 10);      // 10 second timeout before SIGKILL
client.containers.remove(id);
client.containers.remove(id, true);  // force remove even if running

// run — create + start in one call, returns the container ID
std::string id = client.containers.run("alpine");
std::string id = client.containers.run("alpine", {"sh", "-c", "echo hello"});

// bulk operations — run in parallel via the thread pool
std::vector<std::string> ids = {"abc123", "def456", "ghi789"};
client.containers.stop_all(ids, 10);
client.containers.remove_all(ids, true);
```

**Container fields:**
```cpp
struct Container {
    std::string id;
    std::vector<std::string> names;
    std::string image;
    std::string command;
    int64_t created;       // Unix timestamp
    std::string state;     // "running", "exited", "paused", ...
    std::string status;    // human-readable, e.g. "Up 2 hours"
    std::vector<Port> ports;
    std::map<std::string, std::string> labels;
};

struct Port {
    std::string ip;
    int private_port;
    int public_port;
    std::string type;  // "tcp" or "udp"
};
```

---

### Logs

```cpp
// fetch last 100 lines from both stdout and stderr
client.containers.logs(id,
    [](int stream, const std::string& text) {
        if (stream == 1) std::cout << "[stdout] " << text;
        if (stream == 2) std::cerr << "[stderr] " << text;
    }
);

// all options explicit
client.containers.logs(
    id,
    callback,
    /*follow=*/false,   // true = stream live (blocks until container exits)
    /*stdout=*/true,
    /*stderr=*/true,
    /*tail=*/0          // 0 = all lines, N = last N lines
);
```

> **Note:** `follow=true` blocks until the container exits. Non-blocking log
> following requires an epoll-based transport, which is on the roadmap.

---

### Images

```cpp
// pull an image — blocks until the pull completes
client.images.pull("alpine");
client.images.pull("nginx", "1.25");  // specific tag, default is "latest"

// list local images
auto images = client.images.list();
for (auto& img : images)
    for (auto& tag : img.repo_tags)
        std::cout << tag << "\n";

// remove an image
client.images.remove("alpine");
client.images.remove("alpine", true);  // force=true, removes even if in use
```

**Image fields:**
```cpp
struct Image {
    std::string id;
    std::vector<std::string> repo_tags;  // e.g. ["alpine:latest", "alpine:3.18"]
    int64_t created;                     // Unix timestamp
    int64_t size;                        // bytes
};
```

---

### Networks

```cpp
// create a network — returns the network ID
std::string net_id = client.networks.create("my-network");
std::string net_id = client.networks.create("my-network", "bridge", /*internal=*/false);

// list all networks
auto networks = client.networks.list();
for (auto& n : networks)
    std::cout << n.name << "  " << n.driver << "  " << n.scope << "\n";

// connect / disconnect containers
client.networks.connect(net_id, container_id);
client.networks.disconnect(net_id, container_id);
client.networks.disconnect(net_id, container_id, /*force=*/true);

// remove a network
client.networks.remove(net_id);
```

**Network fields:**
```cpp
struct Network {
    std::string id;
    std::string name;
    std::string driver;   // "bridge", "host", "overlay", ...
    std::string scope;    // "local" or "swarm"
    bool internal;
    std::map<std::string, NetworkContainer> containers; // key = container ID
};

struct NetworkContainer {
    std::string name;
    std::string ipv4_address;  // "172.17.0.2/16"
};
```

---

### Volumes

```cpp
// create a volume — returns the full Volume struct
auto vol = client.volumes.create("my-volume");
auto vol = client.volumes.create("my-volume", "local");  // explicit driver
std::cout << vol.mountpoint << "\n";  // /var/lib/docker/volumes/my-volume/_data

// list all volumes
auto volumes = client.volumes.list();
std::cout << volumes.size() << " volumes\n";

// remove a volume — throws docker::ApiError(409) if the volume is in use
client.volumes.remove("my-volume");
```

**Volume fields:**
```cpp
struct Volume {
    std::string name;
    std::string driver;
    std::string mountpoint;
    std::string created_at;  // ISO 8601, e.g. "2024-01-01T00:00:00Z"
    std::string scope;       // "local" or "global"
    std::map<std::string, std::string> labels;
};
```

---

### Error handling

All Docker API errors are thrown as `docker::ApiError`, which inherits from
`std::runtime_error`. You can catch it specifically to branch on the status code,
or catch `std::runtime_error` for generic handling:

```cpp
#include <docker/exception.hpp>

try {
    client.containers.start("nonexistent-id");
} catch (const docker::ApiError& e) {
    if (e.status_code() == 404) {
        std::cout << "Container not found\n";
    } else if (e.status_code() == 304) {
        std::cout << "Already running\n";
    } else {
        throw;  // unexpected — rethrow
    }
} catch (const std::runtime_error& e) {
    // also catches docker::ApiError — for generic logging
    std::cerr << e.what() << "\n";
}
```

Bulk operations collect **all** errors before rethrowing — every task completes
before the first exception propagates:

```cpp
try {
    client.containers.stop_all(ids, 5);
} catch (const docker::ApiError& e) {
    // one or more stops failed — others still completed
    std::cerr << "status " << e.status_code() << ": " << e.message() << "\n";
}
```

---

### Unit testing without a Docker daemon

The `ITransport` interface and `MockTransport` let you test the full API layer
without a running Docker instance:

```cpp
#include <docker/transport/mock_transport.hpp>
#include <docker/api/containers.hpp>

// simplest form — always returns the same response
docker::transport::MockTransport mock(
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 2\r\n"
    "\r\n"
    "[]"
);
docker::api::Containers containers(&mock);
auto list = containers.list();
assert(list.empty());

// advanced form — inspect the request, vary the response
docker::transport::MockTransport mock([](const std::string& request) {
    if (request.find("/containers/json") != std::string::npos)
        return http_ok("[...]");
    return http_err(404, "not found");
});
```

---

## Platform support

| Platform | Transport | Default path | Status |
|----------|-----------|--------------|--------|
| Linux | Unix socket | `/var/run/docker.sock` | Fully supported |
| macOS | Unix socket | `/var/run/docker.sock` | Should work (untested) |
| Windows | Named pipe | `\\.\pipe\docker_engine` | Implemented, untested |

Windows requires Docker Desktop with the named pipe enabled.
Tested contributions welcome.

---

## Architecture

```
docker::Client
├── api::Containers   --> container lifecycle + parallel bulk ops
├── api::Images       --> image management
├── api::Networks     --> network management
└── api::Volumes      --> volume management
         │
         ▼
  http::Request / http::Response
  HTTP/1.1 builder + parser, chunked transfer decoding
         │
         ▼
  transport::ITransport  (interface)
  ├── UnixSocket          --> RAII socket/pipe, connect/send/recv/disconnect
  ├── ThreadPool          --> bounded workers, each with their own transport
  └── MockTransport       --> hardcoded responses for unit testing
         │
         ▼
  /var/run/docker.sock  or  \\.\pipe\docker_engine
  Docker Engine REST API v1.54
```

All communication is HTTP/1.1 over the Docker socket. No third-party HTTP library.
The HTTP layer is ~150 lines of C++17 to keep the dependency footprint minimal.
The only external dependency is nlohmann/json for serialization.

---

## Building and testing

```bash
# configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# build all tests
cmake --build build

# unit tests, no Docker daemon required
./build/unit_test

# integration tests, requires a running Docker daemon
./build/phase6_test   # containers + images
./build/phase7_test   # networks + volumes
./build/phase8_test   # log streaming
```

---

## Roadmap

- [x] Container lifecycle : create, run, start, stop, remove
- [x] Bulk parallel operations via thread pool
- [x] Images : pull, list, remove
- [x] Networks : create, list, connect, disconnect, remove
- [x] Volumes : create, list, remove
- [x] Log streaming with stdout/stderr multiplexing
- [x] Typed error handling : `docker::ApiError` with status code
- [x] `ITransport` abstraction + `MockTransport` for unit testing
- [x] Windows named pipe support (implemented, untested)
- [ ] Typed `exec` API : run commands inside running containers
- [ ] `docker events` : real-time event streaming
- [ ] `docker stats` : live container resource usage
- [ ] Persistent connections with `Connection: keep-alive`
- [ ] epoll-based async transport for non-blocking log following
- [ ] macOS CI verification

---

## Contributing

Contributions welcome, especially:
- macOS testing and verification
- Windows testing with Docker Desktop
- Additional API coverage (exec, events, stats)
- Performance improvements

Please open an issue before starting work on a large feature.

---

## License

MIT
see [LICENSE](LICENSE) for details.
