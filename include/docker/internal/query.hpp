#ifndef DOCKER_INTERNAL_QUERY_HPP
#define DOCKER_INTERNAL_QUERY_HPP

#include <string>
#include <utility>
#include <vector>

namespace docker::internal {
inline std::string
build_query(const std::vector<std::pair<std::string, std::string>> &params) {
  if (params.empty())
    return "";
  std::string q = "?";
  for (auto &[k, v] : params) {
    if (q.size() > 1)
      q += '&';
    q += k + '=' + v;
  }
  return q;
}
} // namespace docker::internal

#endif
