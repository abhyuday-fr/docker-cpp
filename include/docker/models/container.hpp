#ifndef DOCKER_MODELS_CONTAINER_HPP
#define DOCKER_MODELS_CONTAINER_HPP

#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace docker::models {
struct Port {
  std::string ip;
  int private_port{0};
  int public_port{0};
  std::string type; // tcp or udp
}; // struct Port

inline void from_json(const nlohmann::json &j, Port &p) {
  p.ip = j.value("IP", std::string{});
  p.private_port = j.value("PrivatePort", 0);
  p.public_port = j.value("PublicPort", 0);
  p.type = j.value("Type", std::string{});
}

struct Container {
  std::string id;
  std::vector<std::string> names;
  std::string image;
  std::string command;
  int64_t created{0};
  std::string state;
  std::string status;
  std::vector<Port> ports;
  std::map<std::string, std::string> labels;
}; // struct Container

inline void from_json(const nlohmann::json &j, Container &c) {
  j.at("Id").get_to(c.id);
  j.at("Names").get_to(c.names);
  j.at("Image").get_to(c.image);
  j.at("Command").get_to(c.command);
  j.at("Created").get_to(c.created);
  j.at("State").get_to(c.state);
  j.at("Status").get_to(c.status);

  // safe parsing fot Ports (handling both null and missing keys)
  if (j.contains("Ports") && !j.at("Ports").is_null()) {
    j.at("Ports").get_to(c.ports);
  } else {
    c.ports.clear();
  }

  // similarly, safe parsing for Labels
  if (j.contains("Labels") && !j.at("Labels").is_null()) {
    j.at("Labels").get_to(c.labels);
  } else {
    c.labels.clear();
  }
}

} // namespace docker::models

#endif
