#include <ndsl/coroutine/Coroutine.hpp>

using ndsl::coroutine::Coroutine;
using ndsl::coroutine::YieldContext;
using ndsl::coroutine::internal::ndsl_coroutine_wrapper_function;
using ndsl::coroutine::internal::ndsl_start_coroutine;
using ndsl::coroutine::internal::ndsl_swap_to;
using ndsl::framework::EventLoop;
using ndsl::framework::Service;

Coroutine::Coroutine(const MainFunction &func, EventLoop *loop,
                     size_t stack_size)
    : Service(loop),
      stack_size_(stack_size),
      first_call_(true),
      finished_(false),
      main_(func),
      yield_context_(nullptr) {
  yield_context_ = reinterpret_cast<YieldContext *>(new char[stack_size_]);
  yield_context_->main = &main_;
}

Coroutine::~Coroutine() { delete[] reinterpret_cast<char *>(yield_context_); }

void Coroutine::Schedule() {
  if (finished_) {
    SetStatus(Service::STATUS_FINISHED);
    return;
  }

  Resume();
}

Coroutine::ResumeType Coroutine::Resume(uint64_t data, uint16_t cmd) {
  if (yield_context_ == nullptr || finished_) {
    return 0;
  }

  yield_context_->data = data;
  yield_context_->cmd = cmd;

  if (first_call_) {
    first_call_ = false;
    ndsl_start_coroutine(yield_context_, ndsl_coroutine_wrapper_function,
                         yield_context_->stack_base,
                         stack_size_ - sizeof(*yield_context_));
  } else {
    ndsl_swap_to(&yield_context_->caller_sp, yield_context_->self_sp);
  }

  if (yield_context_->cmd & Coroutine::FLAG_FINISHED) {
    finished_ = true;
  }

  return yield_context_->data;
}

uint64_t Coroutine::Yield(YieldContext &context, uint64_t data, uint16_t cmd) {
  context.data = data;
  context.cmd = cmd;

  ndsl_swap_to(&context.self_sp, context.caller_sp);

  // TODO: handle command

  return context.data;
}

extern "C" void ndsl::coroutine::internal::ndsl_coroutine_wrapper_function(
    YieldContext *context) {
  if (context == nullptr) {
    return;
  }

  uint64_t data = (*context->main)(*context);

  Coroutine::Yield(*context, data, Coroutine::FLAG_FINISHED);
}