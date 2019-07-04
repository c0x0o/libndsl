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

int TCPConnection::HandleErrorEvent() {
  RaiseAllReadEvent(-1);
  RaiseAllWriteEvent(-1);

  return 0;
}

int TCPConnection::HandleReadEvent() {
  struct iovec iov[LIBNDSL_NET_MAX_IOV];
  size_t num_vec;

  while (read_requests_.size()) {
    int counter = 0;

    for (ReadRequestEvent::Pointer &eventp : read_requests_) {
      if (counter < LIBNDSL_NET_MAX_IOV) {
        MutableBuffer &buff = eventp->GetBuffer();
        iov[counter].iov_base = buff.data();
        iov[counter].iov_len = buff.size();
        ++counter;
      } else {
        break;
      }
    }

    int nread = ::readv(fd_, iov, counter);

    if (nread < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        AltFlags(0, READABLE);
        break;
      }
      // fatal errror
      SetFlags(RDCLOSED | WRCLOSED | ERROR);
      RaiseAllReadEvent(-1, errno);
      RaiseAllWriteEvent(-1, errno);
      return 0;
    } else if (nread == 0) {
      // local read-end closed == peer write-end closed
      MutableBuffer &buff = read_requests_.front()->GetBuffer();
      AltFlags(RDCLOSED, RDCLOSED | READABLE);
      RaiseReadEvent(buff.original_size() - buff.size());
      RaiseAllReadEvent(0);
      return 0;
    } else {
      int counter = 0;

      // update status of events
      for (ReadRequestEvent::Pointer &eventp : read_requests_) {
        if (counter < LIBNDSL_NET_MAX_IOV && nread > 0) {
          MutableBuffer &buffer = eventp->GetBuffer();
          if (buffer.size() <= nread) {
            nread -= buffer.size();
            buffer += buffer.size();
          } else {
            buffer += nread;
            nread = 0;
          }
          ++counter;
        } else {
          break;
        }
      }

      // raise completed events
      while (read_requests_.size()) {
        MutableBuffer &buffer = read_requests_.front()->GetBuffer();
        if (buffer.size() == 0) {
          RaiseReadEvent(buffer.original_size());
        } else {
          break;
        }
      }
    }
  }

  return read_requests_.size();
}

int TCPConnection::HandleWriteEvent() {
  struct iovec iov[LIBNDSL_NET_MAX_IOV];
  size_t num_vec;

  while (write_requests_.size()) {
    int counter = 0;

    for (WriteRequestEvent::Pointer &eventp : write_requests_) {
      if (counter < LIBNDSL_NET_MAX_IOV) {
        ConstBuffer &buff = eventp->GetBuffer();
        iov[counter].iov_base = const_cast<char *>(buff.data());
        iov[counter].iov_len = buff.size();
        ++counter;
      } else {
        break;
      }
    }

    int nwrite = ::writev(fd_, iov, counter);

    if (nwrite < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        AltFlags(0, WRITEABLE);
        break;
      } else if (errno == ECONNRESET) {
        AltFlags(WRCLOSED, WRITEABLE | WRCLOSED);
        RaiseAllReadEvent(-1, errno);
        RaiseAllWriteEvent(-1, errno);
        return 0;
      } else {
        // fatal errror
        SetFlags(RDCLOSED | WRCLOSED | ERROR);
        RaiseAllReadEvent(-1, errno);
        RaiseAllWriteEvent(-1, errno);
        return 0;
      }
    } else {
      int counter = 0;

      // update status of events
      for (WriteRequestEvent::Pointer &eventp : write_requests_) {
        if (counter < LIBNDSL_NET_MAX_IOV && nwrite > 0) {
          ConstBuffer &buffer = eventp->GetBuffer();
          if (buffer.size() <= nwrite) {
            nwrite -= buffer.size();
            buffer += buffer.size();
          } else {
            buffer += nwrite;
            nwrite = 0;
          }
          ++counter;
        } else {
          break;
        }
      }

      // raise completed events
      while (write_requests_.size()) {
        ConstBuffer &buffer = write_requests_.front()->GetBuffer();
        if (buffer.size() == 0) {
          RaiseWriteEvent(buffer.original_size());
        } else {
          break;
        }
      }
    }
  }

  return write_requests_.size();
}
