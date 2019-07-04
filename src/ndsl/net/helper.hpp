#ifndef LIBNDSL_NET_INTERNAL_HPP_
#define LIBNDSL_NET_INTERNAL_HPP_

#include <fcntl.h>
#include <ndsl/framework/all.hpp>

namespace ndsl {
namespace net {

int SetNonblocking(int fd);

class MutableBuffer {
 public:
  MutableBuffer(void *mem, size_t size)
      : data_(static_cast<char *>(mem)), size_(size), original_size_(size) {}

  char *data() const { return data_; }
  size_t size() const { return size_; }
  size_t original_size() const { return original_size_; }
  void operator+=(size_t len) {
    if (len > size_) return;
    data_ += len;
    size_ -= len;
  }

 private:
  char *data_;
  size_t size_;
  size_t original_size_;
};

class ConstBuffer {
 public:
  ConstBuffer(const void *mem, size_t size)
      : data_(static_cast<const char *>(mem)),
        size_(size),
        original_size_(size) {}

  const char *data() const { return data_; }
  size_t size() const { return size_; }
  size_t original_size() const { return original_size_; }
  void operator+=(size_t len) {
    if (len > size_) return;
    data_ += len;
    size_ -= len;
  }

 private:
  const char *data_;
  size_t size_;
  size_t original_size_;
};

class ReadRequestEvent : public ndsl::framework::Event {
 public:
  using Pointer = std::shared_ptr<ReadRequestEvent>;
  ReadRequestEvent(uint64_t event_id, uint64_t event_type_id,
                   MutableBuffer &buffer)
      : Event(event_id, event_type_id), buffer_(buffer) {}
  MutableBuffer &GetBuffer() { return buffer_; }

 private:
  MutableBuffer buffer_;
};

class WriteRequestEvent : public ndsl::framework::Event {
 public:
  using Pointer = std::shared_ptr<WriteRequestEvent>;
  WriteRequestEvent(uint64_t event_id, uint64_t event_type_id,
                    ConstBuffer &buffer)
      : Event(event_id, event_type_id), buffer_(buffer) {}
  ConstBuffer &GetBuffer() { return buffer_; }

 private:
  ConstBuffer buffer_;
};

}  // namespace net
}  // namespace ndsl

#endif