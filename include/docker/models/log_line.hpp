#ifndef DOCKER_MODELS_LOG_LINE_HPP
#define DOCKER_MODELS_LOG_LINE_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace docker::models {
struct LogLine {
  int stream; // 1 -> stdout, 2 -> stderr
  std::string text;
};

// parsing docker's multiplexed log stream into individual lines
// each frame: [1 byte stream][3 bytes padding][4 bytes size BE][N bytes text]
inline std::vector<LogLine> parse_log_frames(const std::string &raw) {
  std::vector<LogLine> lines;
  std::size_t pos = 0;

  while (pos + 8 <= raw.size()) {
    // byte 0 : stream type
    int stream = static_cast<uint8_t>(raw[pos]);

    // bytes 4-7 : payload size as big-endian uint32_t
    uint32_t size = 0;

    // manually assemble big-endian
    size |= static_cast<uint32_t>(static_cast<uint8_t>(raw[pos + 4])) << 24;
    size |= static_cast<uint32_t>(static_cast<uint8_t>(raw[pos + 5])) << 16;
    size |= static_cast<uint32_t>(static_cast<uint8_t>(raw[pos + 6])) << 8;
    size |= static_cast<uint32_t>(static_cast<uint8_t>(raw[pos + 7]));

    pos += 8; // skipping the header

    if (pos + size > raw.size()) // malformed frame
      break;

    lines.push_back({stream, raw.substr(pos, size)});
    pos += size;
  }

  return lines;
}

} // namespace docker::models

#endif
