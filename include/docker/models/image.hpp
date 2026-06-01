#ifndef DOCKER_MODELS_IMAGE_HPP
#define DOCKER_MODELS_IMAGE_HPP

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace docker::models {
struct Image {
  std::string id;
  std::vector<std::string> repo_tags;
  int64_t created{0};
  int64_t size{0};
};

inline void from_json(const nlohmann::json &j, Image &img) {
  j.at("Id").get_to(img.id);
  img.repo_tags = j.value("RepoTags", std::vector<std::string>{});
  j.at("Created").get_to(img.created);
  j.at("Size").get_to(img.size);
}
} // namespace docker::models

#endif
