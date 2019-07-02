#include <ndsl/net/ipv4/tcp.hpp>

using ndsl::framework::Event;
using ndsl::net::SetNonblocking;
using ndsl::net::ipv4::tcp::ConnectionEvent;
using ndsl::net::ipv4::tcp::TCPConnection;
using ndsl::net::ipv4::tcp::TCPListener;

void TCPListener::RaiseEvent(int result, int error) {
  ConnectionEvent::Pointer eventp = request_list_.front();
  framework::Service *service = eventp->event_source();

  eventp->SetResult(result);
  eventp->SetErrorNumber(error);
  service->HandleResponse(eventp);

  request_list_.pop_front();
}

void TCPListener::RaiseAllEvent(int result, int error) {
  while (request_list_.size()) {
    RaiseEvent(result, error);
  }
}

void TCPListener::HandleErrorEvent() { RaiseAllEvent(-1); }

void TCPListener::HandleReadEvent() {
  while (request_list_.size()) {
    ConnectionEvent::Pointer &eventp = request_list_.front();

    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    int connfd =
        accept(fd_, reinterpret_cast<struct sockaddr *>(&addr), &socklen);
    if (connfd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        AltFlags(0, READABLE);
        break;
      }

      RaiseAllEvent(-1, errno);
      return;
    } else {
      TCPConnection *connection = eventp->connection();

      SetNonblocking(connfd);

      new (connection) TCPConnection(
          connfd, local_, EndPoint(addr.sin_addr.s_addr, addr.sin_port));

      RaiseEvent(connfd);
    }
  }
}