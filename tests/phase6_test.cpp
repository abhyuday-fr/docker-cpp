#include "docker/docker.hpp"
#include <iostream>
#include <string>

int main() {
  docker::Client client;

  // Images
  std::cout << "Pulling alpine...\n";
  client.images.pull("alpine");
  std::cout << "Pull complete\n";

  std::cout << "----Local Images----\n";
  auto imgs = client.images.list();
  for (auto &img : imgs) {
    for (auto &tag : img.repo_tags) {
      std::cout << tag << "\n";
    }
  }

  // single container lifecycle
  std::cout << "\n----Single Container----\n";
  std::string id = client.containers.run("alpine", {"echo", "hello from sdk"});
  std::cout << "Running: " << id.substr(0, 12) << "\n";
  client.containers.stop(id, 5);
  std::cout << "Stopped\n";
  client.containers.remove(id, true);
  std::cout << "Removed\n";

  // bulk operrations
  std::cout << "----Bulk Create + Stop + Remove----\n";
  std::vector<std::string> ids;
  for (int i = 0; i < 10; i++) {
    std::string cid =
        client.containers.create("alpine", "bulk-test-" + std::to_string(i));
    ids.push_back(cid);
    std::cout << "Created: " << cid.substr(0, 12) << "\n";
    client.containers.start(cid);
    std::cout << "Started: " << cid.substr(0, 12) << "\n";
  }

  client.containers.stop_all(ids, 5);
  std::cout << "All stopped\n";

  client.containers.remove_all(ids, true);
  std::cout << "All removed\n";

  return 0;
}
