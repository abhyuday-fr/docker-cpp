#include "docker/docker.hpp"
#include <iostream>

int main() {
  docker::Client client;

  // run a container that produces output on both streams
  std::string id = client.containers.run(
      "alpine",
      {"sh", "-c", "echo hello from stdout; echo error from stderr >&2"});
  std::cout << "Container: " << id.substr(0, 12) << "\n";

  client.containers.stop(id, 5);

  // fetch logs
  std::cout << "\n---- Logs ----\n";
  client.containers.logs(id, [](int stream, const std::string &text) {
    if (stream == 1)
      std::cout << "[stdout] " << text;
    else if (stream == 2)
      std::cerr << "[stderr] " << text;
  });

  client.containers.remove(id, true);
  return 0;
}
