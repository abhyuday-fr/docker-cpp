#include "docker/transport/unix_socket.hpp"
#include <iostream>

int main() {
  docker::transport::UnixSocket sock1;
  docker::transport::UnixSocket sock2 = std::move(sock1);

  std::string request = "GET /v1.54/version HTTP/1.1\r\n"
                        "Host: localhost\r\n"
                        "Connection: close\r\n\r\n";

  // will throw error
  // std::string response1 = sock1.send(request);
  // std::cout << response1 << "\n";

  std::string response2 = sock2.send(request);
  std::cout << response2 << "\n";

  return 0;
}
