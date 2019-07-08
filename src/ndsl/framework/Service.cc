#include <ndsl/framework/all.hpp>

using ndsl::framework::Event;
using ndsl::framework::Service;

thread_local Service *ndsl::framework::current_service = nullptr;
std::atomic<uint64_t> ndsl::framework::Service::next_service_id_(
    Service::USER_USABLE_INIT_ID);

void Service::HandleResponse(Event::Pointer event) {
  auto iter = wait_list_.find(event);
  if (iter != wait_list_.end()) {
    wait_list_.erase(iter);
    if (wait_list_.size() == 0) {
      host_loop_->AddToReadyList(this);
      SetStatus(Service::STATUS_READY);
    }
  }
}

void Service::PushToWaitList(Event::Pointer event) {
  wait_list_.insert(event);
  host_loop_->RemoveFromReadyList(this);
  SetStatus(Service::STATUS_HUNG);
}