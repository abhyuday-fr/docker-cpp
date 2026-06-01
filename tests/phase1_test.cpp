#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main() {
  // creating socket
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return EXIT_FAILURE;
  }

  // set up the address (path to docker.sock)
  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, "/var/run/docker.sock",
          sizeof(addr.sun_path) - 1); // deep copy

  // connect
  if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("connect");
    close(fd);
    return EXIT_FAILURE;
  }

  // send raw http request
  std::string req = "GET /v1.41/version HTTP/1.1\r\n"
                    "Host: localhost\r\n"
                    "Connection: close\r\n\r\n";

  ssize_t sent = send(fd, req.c_str(), req.size(), 0);
  if (sent < 0) {
    perror("send");
    close(fd);
    return EXIT_FAILURE;
  }

  // read the response
  char buf[4096];
  ssize_t n;
  while ((n = recv(fd, buf, sizeof(buf) - 1, 0)) > 0) {
    buf[n] = '\0';
    std::cout << buf;
  }

  if (n < 0)
    perror("recv");

  close(fd);
  return EXIT_SUCCESS;
}
