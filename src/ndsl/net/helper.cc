#include "ndsl/net/helper.hpp"

int ndsl::net::SetNonblocking(int fd) {
  int flag = fcntl(fd, F_GETFL);
  return fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}
