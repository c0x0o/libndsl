#include <iostream>
#include <ndsl/coroutine/Coroutine.hpp>
#include <ndsl/framework/all.hpp>
#include <ndsl/net/ipv4/tcp.hpp>

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

Coroutine::ResumeType echo_server_function(YieldContext &context) {
  EventLoop *loop = EventLoop::GetCurrentEventLoop();
  TCPListener listener;

  int ret = 0;
  EndPoint host("127.0.0.1", 52427);
  listen(host, listener);

  char str[] = "Hello World!\n";
  while (1) {
    TCPConnection *connection = new TCPConnection;

    std::cout << "start accepting..." << std::endl;
    ret = accept(&listener, connection, context);
    if (ret > 0) {
      std::cout << ret << " = " << connection->fd() << std::endl;
    } else {
      std::cout << "Error occured: " << errno << std::endl; 
    }

    loop->WatchPollerEvent(connection);
    std::cout << "Start writing" << std::endl;

    write(connection, ConstBuffer(str, sizeof(str) + 1), context);
    std::cout << "write complete" << std::endl;
    close(connection);

    delete connection;
  }
}

int main() {
  EventLoop loop;

  // std::cout << "start!" << std::endl;

  loop.MakeService<Coroutine>(Coroutine::TypeId, echo_server_function);

  loop.Loop();

  return 0;
}