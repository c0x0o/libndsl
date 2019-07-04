#ifndef LIBNDSL_NET_IPV4_TCP_HPP_
#define LIBNDSL_NET_IPV4_TCP_HPP_

#include <sys/uio.h>

#include <ndsl/coroutine/Coroutine.hpp>
#include <ndsl/framework/all.hpp>
#include <ndsl/net/helper.hpp>
#include <ndsl/net/ipv4/EndPoint.hpp>

namespace ndsl {
namespace net {
namespace ipv4 {
namespace tcp {

class TCPConnection;
class TCPService;
class TCPListener;
class ConnectionEvent;

class TCPConnection : public framework::Pollable {
 public:
  using Pointer = std::shared_ptr<TCPConnection>;
  using ReadRequestList = std::list<ReadRequestEvent::Pointer>;
  using WriteRequestList = std::list<WriteRequestEvent::Pointer>;

  TCPConnection() : Pollable(-1) {}
  TCPConnection(int fd, const EndPoint &local, const EndPoint &peer)
      : Pollable(fd), local_(local), peer_(peer) {}

  const ipv4::EndPoint &local() const { return local_; }
  const ipv4::EndPoint &peer() const { return peer_; }

  int HandleReadEvent() override;
  int HandleWriteEvent() override;
  int HandleErrorEvent() override;

  friend ReadRequestEvent::Pointer read(TCPConnection *connection,
                                        MutableBuffer buffer);
  friend WriteRequestEvent::Pointer write(TCPConnection *connection,
                                          ConstBuffer buffer);

 private:
  void RaiseReadEvent(int result, int error = 0);
  void RaiseWriteEvent(int result, int error = 0);
  void RaiseAllReadEvent(int result, int error = 0);
  void RaiseAllWriteEvent(int result, int error = 0);

  ipv4::EndPoint local_;
  ipv4::EndPoint peer_;
  ReadRequestList read_requests_;
  WriteRequestList write_requests_;
};

class ConnectionEvent : public framework::Event {
 public:
  using Pointer = std::shared_ptr<ConnectionEvent>;

  ConnectionEvent(uint64_t id, uint64_t type_id, TCPConnection *connp)
      : Event(id, type_id), connection_(connp) {}

  TCPConnection *connection() { return connection_; }

 private:
  TCPConnection *connection_;
};

class TCPListener : public framework::Pollable {
 public:
  using Pointer = std::shared_ptr<TCPListener>;
  using RequestList = std::list<ConnectionEvent::Pointer>;

  TCPListener() : Pollable(-1) {}
  TCPListener(int fd, ipv4::EndPoint &local)
      : Pollable(fd, EPOLLIN | EPOLLET), local_(local) {}

  const EndPoint &local() const { return local_; }

  int HandleErrorEvent() override;
  int HandleReadEvent() override;

  friend ConnectionEvent::Pointer accept(TCPListener *listener,
                                         TCPConnection *connection);

 private:
  void RaiseEvent(int result, int error = 0);
  void RaiseAllEvent(int result, int error = 0);

  ipv4::EndPoint local_;
  RequestList request_list_;
};

// synchronous operation
int listen(ipv4::EndPoint &local, TCPListener &pointer);
int connect(ipv4::EndPoint &peer, ipv4::EndPoint &local,
            TCPConnection &connection);

// asynchronous operation
ConnectionEvent::Pointer accept(TCPListener *listener,
                                TCPConnection *connection);
int accept(TCPListener *listener, TCPConnection *connection,
           coroutine::YieldContext &context);
ReadRequestEvent::Pointer read(TCPConnection *connection, MutableBuffer buffer);
int read(TCPConnection *connection, MutableBuffer buffer,
         coroutine::YieldContext &context);
WriteRequestEvent::Pointer write(TCPConnection *connection, ConstBuffer buffer);
int write(TCPConnection *connection, ConstBuffer buffer,
          coroutine::YieldContext &context);
void close(TCPListener *listener);
void close(TCPConnection *connection);

}  // namespace tcp
}  // namespace ipv4
}  // namespace net
}  // namespace ndsl

#endif