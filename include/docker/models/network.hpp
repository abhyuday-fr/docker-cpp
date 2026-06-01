#ifndef DOCKER_MODELS_NETWORK_HPP
#define DOCKER_MODELS_NETWORK_HPP

#include <map>
#include <nlohmann/json.hpp>
#include <string>

namespace docker::models {
struct NetworkContainer {
  std::string name;
  std::string ipv4_address;
}; // struct Network Container

inline void from_json(const nlohmann::json &j, NetworkContainer &n) {
  n.name = j.value("Name", std::string{});
  n.ipv4_address = j.value("IPv4Address", std::string{});
}

struct Network {
  std::string id;
  std::string name;
  std::string driver;
  std::string scope;
  bool internal{false};
  std::map<std::string, NetworkContainer>
      containers; // container ID : endpoint info
}; // struct Network

inline void from_json(const nlohmann::json &j, Network &n) {
  j.at("Id").get_to(n.id);
  j.at("Name").get_to(n.name);
  n.driver = j.value("Driver", std::string{});
  n.scope = j.value("Scope", std::string{});
  n.internal = j.value("Internal", false);

  // containers keys may be missing or null on networks with no active
  // containers
  if (j.contains("Containers") && !j.at("Containers").is_null())
    j.at("Containers").get_to(n.containers);
  else
    n.containers.clear();
}

} // namespace docker::models

#endif
