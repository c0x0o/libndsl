#include <ndsl/net/ipv4/tcp.hpp>

using ndsl::framework::Event;
using ndsl::net::ConstBuffer;
using ndsl::net::MutableBuffer;
using ndsl::net::ReadRequestEvent;
using ndsl::net::WriteRequestEvent;
using ndsl::net::ipv4::EndPoint;
using ndsl::net::ipv4::tcp::ConnectionEvent;
using ndsl::net::ipv4::tcp::TCPConnection;

void TCPConnection::RaiseReadEvent(int result, int error) {
  ReadRequestEvent::Pointer &eventp = read_requests_.front();
  framework::Service *service = eventp->event_source();

  eventp->SetErrorNumber(error);
  eventp->SetResult(result);
  service->HandleResponse(eventp);

  read_requests_.pop_front();
}

void TCPConnection::RaiseWriteEvent(int result, int error) {
  WriteRequestEvent::Pointer &eventp = write_requests_.front();
  framework::Service *service = eventp->event_source();

  eventp->SetErrorNumber(error);
  eventp->SetResult(result);
  service->HandleResponse(eventp);

  write_requests_.pop_front();
}

void TCPConnection::RaiseAllReadEvent(int result, int error) {
  while (read_requests_.size()) {
    RaiseReadEvent(result, error);
  }
}

void TCPConnection::RaiseAllWriteEvent(int result, int error) {
  while (write_requests_.size()) {
    RaiseWriteEvent(result, error);
  }
}

void TCPConnection::HandleErrorEvent() {
  RaiseAllReadEvent(-1);
  RaiseAllWriteEvent(-1);
}

void TCPConnection::HandleReadEvent() {
  while (read_requests_.size()) {
    ReadRequestEvent::Pointer &eventp = read_requests_.front();
    MutableBuffer &buff = eventp->GetBuffer();

    int nread = 0;
    int total = 0;

    do {
      nread = ::read(fd_, buff.data(), buff.size());

      if (nread < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          AltFlags(0, READABLE);
          break;
        }
        // fatal errror
        SetFlags(RDCLOSED | WRCLOSED | ERROR);
        RaiseAllReadEvent(-1, errno);
        RaiseAllWriteEvent(-1, errno);

        return;
      } else if (nread == 0) {
        // read end closed
        AltFlags(RDCLOSED, RDCLOSED | READABLE);
        RaiseAllReadEvent(total);
        return;
      } else {
        buff += nread;
        total += nread;

        if (buff.size() == 0) {
          RaiseReadEvent(total);
          break;
        }
      }
    } while (nread > 0);
  }
}

void TCPConnection::HandleWriteEvent() {
  while (write_requests_.size()) {
    WriteRequestEvent::Pointer &eventp = write_requests_.front();
    ConstBuffer &buff = eventp->GetBuffer();

    int nwrite = 0;
    int total = 0;

    do {
      nwrite = ::write(fd_, buff.data(), buff.size());

      if (nwrite < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          AltFlags(0, WRITEABLE);
          break;
        } else if (errno == ECONNRESET) {
          AltFlags(WRCLOSED, WRITEABLE | WRCLOSED);
          RaiseAllReadEvent(-1, errno);
          RaiseAllWriteEvent(-1, errno);
          break;
        } else {
          // fatal errror
          SetFlags(RDCLOSED | WRCLOSED | ERROR);
          RaiseAllReadEvent(-1, errno);
          RaiseAllWriteEvent(-1, errno);
          return;
        }
      } else {
        buff += nwrite;
        total += nwrite;

        if (buff.size() == 0) {
          RaiseWriteEvent(nwrite);
          break;
        }
      }
    } while (nwrite > 0);
  }
}
