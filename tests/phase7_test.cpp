#include "docker/docker.hpp"
#include <iostream>

int main() {
  docker::Client client;

  // Networks
  std::cout << "---- NEtworks ----\n";

  std::string net_id = client.networks.create("sdk-test-net", "bridge");
  std::cout << "Created network: " << net_id.substr(0, 12) << "\n";

  // list networks finding our new one
  auto nets = client.networks.list();
  for (auto &n : nets) {
    if (n.name == "sdk-test-net")
      std::cout << "Listed: " << n.name << " driver=" << n.driver << "\n";
  }

  // create a container and connect it to our network
  std::string cid = client.containers.create("alpine", "sdk-net-test");
  std::cout << "Created container: " << cid.substr(0, 12) << "\n";

  client.networks.connect(net_id, cid);
  std::cout << "Connected container to network\n";

  client.networks.disconnect(net_id, cid, true);
  std::cout << "Disconnected\n";

  // cleanup
  client.containers.remove(cid, true);
  client.networks.remove(net_id);
  std::cout << "Network Cleaned up\n";

  // Volumes
  std::cout << "---- Volumes ----\n";

  auto vol = client.volumes.create("sdk-test-vol");
  std::cout << "Created volume: " << vol.name << "\n";
  std::cout << "Mountpoint: " << vol.mountpoint << "\n";

  auto vols = client.volumes.list();
  std::cout << "Total Volumes: " << vols.size() << "\n";

  client.volumes.remove("sdk-test-vol");
  std::cout << "Volume removed\n";

  return 0;
}
