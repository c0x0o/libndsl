#ifndef LIBNDSL_FRAMEWORK_ALL_HPP_
#define LIBNDSL_FRAMEWORK_ALL_HPP_

#include <errno.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <ndsl/config.hpp>

namespace ndsl {
namespace framework {

class EventLoop;
class Event;
class Pollable;
class Service;

extern thread_local EventLoop *current_event_loop;
extern thread_local Service *current_service;

class Pollable {
 public:
  using Pointer = std::shared_ptr<Pollable>;
  enum {
    READABLE = 1,
    WRITEABLE = 2,
    ERROR = 4,
    RDCLOSED = 8,
    WRCLOSED = 16,
    DEFAULT_FLAGS = 0
  };
  Pollable(int fd = -1, int events = EPOLLIN | EPOLLOUT | EPOLLET,
           uint32_t flags = DEFAULT_FLAGS)
      : flags_(DEFAULT_FLAGS), fd_(fd), events_(events) {}

  operator bool() {
    if (fd_ < 0) {
      return false;
    } else {
      return true;
    }
  }

  int fd() const { return fd_; }
  int events() const { return events_; }
  bool readable() const { return flags_ & READABLE; }
  bool writeable() const { return flags_ & WRITEABLE; }
  bool error() const { return flags_ & ERROR; }
  bool rdclosed() const { return flags_ & RDCLOSED; }
  bool wrclosed() const { return flags_ & WRCLOSED; }
  EventLoop *host_loop() const { return host_loop_; }

  void SetFlags(uint32_t flags) { flags_ = flags; }
  void AltFlags(uint32_t flags, uint32_t mask) {
    flags_ = (flags & mask) | (flags_ & ~mask);
  }
  void SetEvents(int events) { events_ = events; }
  void SetHostLoop(EventLoop *loop) { host_loop_ = loop; }

  virtual int HandleErrorEvent() { return 0; }
  virtual int HandleReadEvent() { return 0; }
  virtual int HandleWriteEvent() { return 0; }

 protected:
  EventLoop *host_loop_;
  uint32_t flags_;
  int fd_;
  int events_;
};

class Event {
 public:
  using Pointer = std::shared_ptr<Event>;
  Event(uint64_t event_id, uint64_t event_type_id)
      : event_type_id_(event_type_id),
        event_id_(event_id),
        errno_(0),
        result_(0) {}

  uint64_t event_type_id() const { return event_type_id_; }
  uint64_t event_id() const { return event_id_; }
  Service *event_source() const {
    return reinterpret_cast<Service *>(event_type_id_);
  }
  int error_number() const { return errno_; }
  int result() const { return result_; }

  void SetErrorNumber(int num) { errno_ = num; }
  void SetResult(int result) { result_ = result; }

 protected:
  int errno_;
  int result_;

 private:
  const uint64_t event_type_id_;
  const uint64_t event_id_;
};

bool operator==(const Event &ev1, const Event &ev2);
bool operator>(const Event &ev1, const Event &ev2);
bool operator<(const Event &ev1, const Event &ev2);
bool operator>=(const Event &ev1, const Event &ev2);
bool operator<=(const Event &ev1, const Event &ev2);

class EventPointerCompare {
 public:
  bool operator()(const Event::Pointer &ep1, const Event::Pointer &ep2) const {
    if (*ep1 < *ep2) {
      return true;
    }

    return false;
  }
};

class Service : public std::enable_shared_from_this<Service> {
 public:
  enum Status { STATUS_HUNG, STATUS_READY, STATUS_FINISHED };
  enum { USER_USABLE_INIT_ID = 128 };
  using Pointer = std::shared_ptr<Service>;
  using WaitList = std::unordered_set<Event::Pointer>;

  static inline Service *GetCurrentService() { return current_service; }
  static inline Service *SetCurrentService(Service *s) {
    return current_service = s;
  }
  static uint64_t next_service_id() { return next_service_id_; }

  Service(EventLoop *host)
      : service_id_(next_service_id_++),
        status_(STATUS_READY),
        host_loop_(host),
        next_event_id_(0) {}
  virtual ~Service() = default;
  Service(const Service &service) = delete;
  Service &operator=(const Service &service) = delete;

  uint64_t service_id() const { return service_id_; }
  enum Status status() const { return status_; }
  EventLoop *host_loop() const { return host_loop_; }

  enum Status SetStatus(enum Status stat) { return status_ = stat; }

  template <typename EventType, typename... Args>
  std::shared_ptr<EventType> GenerateEvent(Args... args) {
    std::shared_ptr<EventType> eventp = std::make_shared<EventType>(
        next_event_id_++, reinterpret_cast<uint64_t>(this), args...);
    SetStatus(STATUS_HUNG);
    PushToWaitList(eventp);

    return eventp;
  }

  virtual void PushToWaitList(Event::Pointer event);
  virtual void HandleResponse(Event::Pointer event);
  virtual void Schedule() {}

 protected:
  // save events which may raise this Service from STATUS_HANG
  WaitList wait_list_;

 private:
  static std::atomic<uint64_t> next_service_id_;
  const uint64_t service_id_;
  enum Status status_;
  EventLoop *host_loop_;
  uint64_t next_event_id_;
};

class EventLoop : public std::enable_shared_from_this<EventLoop> {
 public:
  using Pointer = std::shared_ptr<EventLoop>;
  using ServiceMap = std::map<uint64_t, Service *>;
  using ReadyList = std::list<Service *>;

  static inline EventLoop *GetCurrentEventLoop() { return current_event_loop; }
  static inline EventLoop *SetCurrentEventLoop(EventLoop *l) {
    return current_event_loop = l;
  }

  EventLoop() : running_(false) {
    if (current_event_loop == nullptr) {
      current_event_loop = this;
    }

    epfd_ = epoll_create1(0);
    next_loop_timeout_ = 0;
  }

  inline bool running() { return running_.load(std::memory_order_relaxed); }

  template <class ServiceType, typename... Args>
  ServiceType *MakeService(Args... args) {
    ServiceMap::iterator iter = services_.find(Service::next_service_id());

    if (iter == services_.end()) {
      ServiceType *s = new ServiceType(args...);
      services_.insert(std::make_pair(s->service_id(), s));
      AddToReadyList(s);

      return s;
    } else {
      return nullptr;
    }
  }
  template <class ServiceType>
  ServiceType *GetService(uint64_t id) {
    ServiceMap::iterator map_iter = services_.find(id);

    if (map_iter == services_.end()) {
      return nullptr;
    }

    return reinterpret_cast<ServiceType *>(map_iter->second);
  }
  void DestroyService(uint64_t id);
  void AddToReadyList(Service *s);
  void RemoveFromReadyList(Service *s);
  int WatchPollerEvent(Pollable *pollable);
  int UnwatchPollerEvent(Pollable *pollable);
  int ModifyPollerEvent(Pollable *pollable);
  void Stop() { running_.store(false, std::memory_order_relaxed); }
  void RunOneTick();
  void Loop();

 private:
  std::atomic<bool> running_;
  ServiceMap services_;
  ReadyList ready_list_;
  int epfd_;
  int next_loop_timeout_;
  uint64_t next_event_id_;
};

}  // namespace framework
}  // namespace ndsl

#endif