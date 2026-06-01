#include "docker/api/containers.hpp"
#include "docker/transport/unix_socket.hpp"
#include <iostream>

int main() {
  docker::transport::UnixSocket sock;
  docker::api::Containers containers(sock);

  // listing all containers
  auto list = containers.list(true);
  std::cout << "=== Containers ===\n";
  for (auto &c : list) {
    std::cout << c.id.substr(0, 12) << " " << c.image << " " << c.status
              << "\n";
  }

  // create and start one
  std::string id = containers.create("alpine", "sdk-test");
  std::cout << "\nCreated: " << id.substr(0, 12) << "\n";

  containers.start(id);
  std::cout << "Started\n";

  containers.stop(id, 5);
  std::cout << "Stopped\n";

  containers.remove(id, true);
  std::cout << "Removed\n";
}
