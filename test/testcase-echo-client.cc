#include <functional>
#include <iostream>

#include <ndsl/coroutine/Coroutine.hpp>
#include <ndsl/framework/all.hpp>
#include <ndsl/net/ipv4/tcp.hpp>

uint64_t packet_size = 2048;
uint64_t connection_num = 1;

using ndsl::coroutine::Coroutine;
using ndsl::coroutine::YieldContext;
using ndsl::framework::EventLoop;
using ndsl::framework::Service;
using ndsl::net::ConstBuffer;
using ndsl::net::MutableBuffer;
using ndsl::net::ipv4::EndPoint;
using ndsl::net::ipv4::tcp::TCPConnection;
using ndsl::net::ipv4::tcp::TCPListener;
using ndsl::net::ipv4::tcp::TCPService;

Coroutine::ResumeType connection_service(YieldContext &context,
                                         TCPConnection *connection) {
  char *buffer = new char[packet_size];
  std::cout << "connection start !" << std::endl;

  while (1) {
    std::cout << "connection writing data" << std::endl;
    if (!connection->wrclosed()) {
      int nwrite = write(connection, ConstBuffer(buffer, packet_size), context);
      if (nwrite < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          std::cout << "You are not supposed to be here" << std::endl;
        }
        std::cout << "read error occured: " << errno << std::endl;
        goto EXIT;
      }
      std::cout << nwrite << " bytes data sent" << std::endl;
    } else {
      std::cout << "write closed" << std::endl;
      goto EXIT;
    }

    std::cout << "connection waiting data" << std::endl;
    if (!connection->rdclosed()) {
      int nread = read(connection, MutableBuffer(buffer, packet_size), context);
      if (nread < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          std::cout << "You are not supposed to be here" << std::endl;
        }
        std::cout << "read error occured: " << errno << std::endl;
        goto EXIT;
      } else if (nread > 0) {
        std::cout << nread << " bytes data received" << std::endl;
      }
    } else {
      std::cout << "read closed" << std::endl;
      goto EXIT;
    }
    // just an example: elegant exit
    // will never be excuted
    if (connection->rdclosed() && connection->wrclosed()) {
      std::cout << "elegant quit" << std::endl;
      goto EXIT;
    }
  }

EXIT:
  std::cout << "connection closed !" << std::endl;
  close(connection);
  delete connection;
  return 0;
}

Coroutine::ResumeType echo_client_function(YieldContext &context) {
  EventLoop *loop = EventLoop::GetCurrentEventLoop();

  int ret = 0;
  EndPoint server("127.0.0.1", 52427);
  EndPoint peer;

  TCPConnection *connection = new TCPConnection;

  for (int i = 0; i < connection_num; ++i) {
    ret = connect(server, peer, *connection);
    if (ret < 0) {
      std::cout << "Error occured: " << errno << std::endl;
      close(connection);
      return 0;
    }

    loop->WatchPollerEvent(connection);
    loop->MakeService<Coroutine>(
        Coroutine::TypeId,
        std::bind(connection_service, std::placeholders::_1, connection));
  }
  return 0;
}

int main() {
  EventLoop loop;

  std::cout << "Input Packet Size: ";
  std::cin >> packet_size;
  std::cout << "Client started" << std::endl;
  std::cout << "Input connection number: ";
  std::cin >> connection_num;

  loop.MakeService<Coroutine>(Coroutine::TypeId, echo_client_function);

  loop.Loop();

  return 0;
}