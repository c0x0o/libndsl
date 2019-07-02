#include <ndsl/net/ipv4/EndPoint.hpp>

using ndsl::net::ipv4::EndPoint;

bool ndsl::net::ipv4::operator<(const EndPoint &a, const EndPoint &b) {
  if (a.GetNetOrderAddress() < b.GetNetOrderAddress()) {
    return true;
  } else if (a.GetNetOrderPort() < b.GetNetOrderPort()) {
    return true;
  }

  return false;
}

bool ndsl::net::ipv4::operator==(const EndPoint &a, const EndPoint &b) {
  if (a.GetNetOrderAddress() == b.GetNetOrderAddress() &&
      a.GetNetOrderPort() == b.GetNetOrderPort()) {
    return true;
  }

  return false;
}
