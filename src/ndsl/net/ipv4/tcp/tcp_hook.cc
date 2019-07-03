#include <ndsl/net/ipv4/tcp.hpp>

using ndsl::coroutine::Coroutine;
using ndsl::coroutine::YieldContext;
using ndsl::framework::Event;
using ndsl::framework::EventLoop;
using ndsl::framework::Pollable;
using ndsl::framework::Service;
using ndsl::net::ReadRequestEvent;
using ndsl::net::SetNonblocking;
using ndsl::net::WriteRequestEvent;
using ndsl::net::ipv4::EndPoint;
using ndsl::net::ipv4::tcp::ConnectionEvent;
using ndsl::net::ipv4::tcp::TCPConnection;
using ndsl::net::ipv4::tcp::TCPListener;

int ndsl::net::ipv4::tcp::listen(net::ipv4::EndPoint &local,
                                 TCPListener &listener) {
  int fd, ret;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    return fd;
  }

  struct sockaddr_in addr;
  local.FillInetStructure(&addr);
  ret = bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  if (ret < 0) {
    ::close(fd);
    return ret;
  }

  ret = ::listen(fd, LIBNDSL_NET_BACKLOG);
  if (ret < 0) {
    ::close(fd);
    return ret;
  }

  SetNonblocking(fd);

  new (&listener) TCPListener(fd, local);

  EventLoop::GetCurrentEventLoop()->WatchPollerEvent(&listener);

  return 0;
}

int ndsl::net::ipv4::tcp::connect(EndPoint &peer, EndPoint &local,
                                  TCPConnection &connection) {
  int fd, ret;
  struct sockaddr_in local_in, peer_in;
  socklen_t socklen = sizeof(local_in);

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    return fd;
  }

  local.FillInetStructure(&local_in);
  peer.FillInetStructure(&peer_in);
  ret = bind(fd, reinterpret_cast<struct sockaddr *>(&local_in),
             sizeof(local_in));
  if (ret < 0) {
    ::close(fd);
    return ret;
  }

  ret = connect(fd, reinterpret_cast<struct sockaddr *>(&peer_in),
                sizeof(peer_in));
  if (ret < 0) {
    ::close(fd);
    return ret;
  }

  new (&connection) TCPConnection(fd, local, peer);

  SetNonblocking(fd);

  return 0;
}

ConnectionEvent::Pointer ndsl::net::ipv4::tcp::accept(
    TCPListener *listener, TCPConnection *connection) {
  Service *current_service = Service::GetCurrentService();
  ConnectionEvent::Pointer eventp =
      current_service->GenerateEvent<ConnectionEvent>(connection);

  listener->request_list_.push_back(eventp);

  return eventp;
}

int ndsl::net::ipv4::tcp::accept(TCPListener *listener,
                                 TCPConnection *connection,
                                 YieldContext &context) {
  ConnectionEvent::Pointer evp = accept(listener, connection);

  if (listener->readable()) {
    int remained = listener->HandleReadEvent();

    if (remained > 0) {
      Coroutine::Yield(context);
    }
  } else {
    Coroutine::Yield(context);
  }

  errno = evp->error_number();
  return evp->result();
}

ReadRequestEvent::Pointer ndsl::net::ipv4::tcp::read(TCPConnection *connection,
                                                     MutableBuffer buffer) {
  Service *current_service = Service::GetCurrentService();
  ReadRequestEvent::Pointer event =
      current_service->GenerateEvent<ReadRequestEvent>(buffer);

  connection->read_requests_.push_back(event);

  return event;
}

int ndsl::net::ipv4::tcp::read(TCPConnection *connection, MutableBuffer buffer,
                               YieldContext &context) {
  ReadRequestEvent::Pointer evp = read(connection, buffer);

  if (connection->readable()) {
    int remained = connection->HandleReadEvent();

    if (remained > 0) {
      Coroutine::Yield(context);
    }
  } else {
    Coroutine::Yield(context);
  }

  errno = evp->error_number();
  return evp->result();
}

WriteRequestEvent::Pointer ndsl::net::ipv4::tcp::write(
    TCPConnection *connection, ConstBuffer buffer) {
  Service *current_service = Service::GetCurrentService();
  WriteRequestEvent::Pointer event =
      current_service->GenerateEvent<WriteRequestEvent>(buffer);

  connection->write_requests_.push_back(event);

  return event;
}

int ndsl::net::ipv4::tcp::write(TCPConnection *connection, ConstBuffer buffer,
                                YieldContext &context) {
  WriteRequestEvent::Pointer evp = write(connection, buffer);

  if (connection->writeable()) {
    int remained = connection->HandleWriteEvent();

    if (remained > 0) {
      Coroutine::Yield(context);
    }
  } else {
    Coroutine::Yield(context);
  }

  errno = evp->error_number();
  return evp->result();
}

void ndsl::net::ipv4::tcp::close(TCPListener *listener) {
  EventLoop::GetCurrentEventLoop()->UnwatchPollerEvent(listener);

  ::close(listener->fd());
}

void ndsl::net::ipv4::tcp::close(TCPConnection *connection) {
  EventLoop::GetCurrentEventLoop()->UnwatchPollerEvent(connection);

  ::close(connection->fd());
}