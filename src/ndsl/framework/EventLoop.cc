#include <ndsl/framework/all.hpp>

using ndsl::framework::current_service;
using ndsl::framework::EventLoop;
using ndsl::framework::Pollable;
using ndsl::framework::Service;

thread_local EventLoop *ndsl::framework::current_event_loop = nullptr;

void EventLoop::DestroyService(uint64_t id) {
  ServiceMap::iterator map_iter = services_.find(id);

  if (map_iter == services_.end()) {
    return;
  } else {
    delete map_iter->second;
    services_.erase(map_iter);
  }
}

void EventLoop::RunOneTick() {
  int ev_num;
  struct epoll_event events[LIBNDSL_FRAMEWORK_MAX_EVENT_PER_LOOP];

  ev_num = epoll_wait(epfd_, events, LIBNDSL_FRAMEWORK_MAX_EVENT_PER_LOOP,
                      next_loop_timeout_);

  for (int i = 0; i < ev_num; ++i) {
    Pollable *pollable = static_cast<Pollable *>(events[i].data.ptr);

    if (events[i].events & EPOLLERR || events[i].events & EPOLLPRI) {
      pollable->SetFlags(Pollable::RDCLOSED | Pollable::WRCLOSED |
                         Pollable::ERROR);
      pollable->HandleErrorEvent();
    }

    if (events[i].events & EPOLLIN) {
      pollable->AltFlags(Pollable::READABLE, Pollable::READABLE);
      pollable->HandleReadEvent();
    }

    if (events[i].events & EPOLLOUT) {
      pollable->AltFlags(Pollable::WRITEABLE, Pollable::WRITEABLE);
      pollable->HandleWriteEvent();
    }
  }

  for (auto i = services_.begin(); i != services_.end();) {
    Service *s = i->second;

    if (s->status() == Service::STATUS_READY) {
      Service::SetCurrentService(s);
      s->Schedule();
    }

    if (s->status() == Service::STATUS_FINISHED) {
      auto temp = i;
      ++i;
      services_.erase(temp);
    } else {
      ++i;
    }
  }
}

void EventLoop::Loop() {
  running_.store(true, std::memory_order_relaxed);
  while (running_.load(std::memory_order_relaxed)) {
    RunOneTick();
  }
}

int EventLoop::WatchPollerEvent(Pollable *pollable) {
  struct epoll_event ev;

  ev.events = pollable->events();
  ev.data.ptr = pollable;
  pollable->SetHostLoop(this);

  return epoll_ctl(epfd_, EPOLL_CTL_ADD, pollable->fd(), &ev);
}

int EventLoop::UnwatchPollerEvent(Pollable *pollable) {
  pollable->SetHostLoop(nullptr);
  return epoll_ctl(epfd_, EPOLL_CTL_DEL, pollable->fd(), nullptr);
}

int EventLoop::ModifyPollerEvent(Pollable *pollable) {
  struct epoll_event ev;

  ev.events = pollable->events();
  ev.data.ptr = pollable;

  return epoll_ctl(epfd_, EPOLL_CTL_MOD, pollable->fd(), &ev);
}