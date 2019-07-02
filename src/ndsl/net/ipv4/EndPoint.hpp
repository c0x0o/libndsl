#ifndef LIBNDSL_NET_IPV4_ENDPOINT_HPP_
#define LIBNDSL_NET_IPV4_ENDPOINT_HPP_

#include <arpa/inet.h>
#include <netinet/in.h>

#include <cstdint>
#include <string>

namespace ndsl {
namespace net {
namespace ipv4 {

class EndPoint {
 public:
  EndPoint() : addr_(0), port_(0) {}
  EndPoint(const std::string &ipv4, uint16_t port) {
    inet_pton(AF_INET, ipv4.data(), &addr_);
    port_ = htons(port);
  }
  EndPoint(uint32_t addr_net, uint16_t port_net)
      : addr_(addr_net), port_(port_net) {}

  uint16_t GetNetOrderPort() const { return port_; }
  uint32_t GetNetOrderAddress() const { return addr_; }
  struct sockaddr *FillInetStructure(struct sockaddr_in *holder) {
    holder->sin_family = AF_INET;
    holder->sin_addr.s_addr = addr_;
    holder->sin_port = port_;

    return reinterpret_cast<struct sockaddr *>(holder);
  }

 private:
  uint32_t addr_;
  uint16_t port_;
};

bool operator<(const EndPoint &a, const EndPoint &b);
bool operator==(const EndPoint &a, const EndPoint &b);

}  // namespace ipv4
}  // namespace net
}  // namespace ndsl

#endif