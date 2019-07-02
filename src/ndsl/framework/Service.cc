#include <ndsl/framework/all.hpp>

using ndsl::framework::Event;
using ndsl::framework::Service;

thread_local Service *ndsl::framework::current_service = nullptr;

void Service::HandleResponse(Event::Pointer event) {
  if (wait_list_.count(event) > 0) {
    wait_list_.erase(event);
    if (wait_list_.size() == 0) {
      SetStatus(Service::STATUS_READY);
    }
  }
}

void Service::PushToWaitList(Event::Pointer event) {
  wait_list_.insert(event);
  SetStatus(Service::STATUS_HUNG);
}