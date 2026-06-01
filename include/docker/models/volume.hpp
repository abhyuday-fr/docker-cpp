#ifndef DOCKER_MODELS_VOLUME_HPP
#define DOCKER_MODELS_VOLUME_HPP

#include <map>
#include <nlohmann/json.hpp>
#include <string>

namespace docker::models {

struct Volume {
  std::string name;
  std::string driver;
  std::string mountpoint;
  std::string created_at;
  std::string scope;
  std::map<std::string, std::string> labels;
}; // Struct Volume

inline void from_json(const nlohmann::json &j, Volume &v) {
  j.at("Name").get_to(v.name);
  j.at("Driver").get_to(v.driver);
  j.at("Mountpoint").get_to(v.mountpoint);
  v.created_at = j.value("CreatedAt", std::string{});
  v.scope = j.value("Scope", std::string{});

  if (j.contains("Labels") && !j.at("Labels").is_null())
    j.at("Labels").get_to(v.labels);
  else
    v.labels.clear();
}
} // namespace docker::models

#endif
