#ifndef LIBNDSL_COROUTINE_COROUTINE_HPP_
#define LIBNDSL_COROUTINE_COROUTINE_HPP_

#include <functional>
#include <memory>

#include <ndsl/config.hpp>
#include <ndsl/framework/all.hpp>

namespace ndsl {
namespace coroutine {

class Coroutine;
struct YieldContext;

// 1. not thread safe:
//    all member function and data in Coroutine are not thread safe,
//    but Coroutine can be scheduled across different threads.
class Coroutine : public framework::Service {
 public:
  using ResumeType = uint64_t;
  using MainFunction = std::function<ResumeType(YieldContext &)>;
  using Pointer = std::shared_ptr<Coroutine>;

  enum { CMD_TERMINATE = 0x1 };
  enum { FLAG_FINISHED = 0x8000 };
  enum { TypeId = 1 };

  const constexpr static size_t default_stack_size =
      LIBNDSL_COROUTINE_STACK_SIZE;

  static uint64_t Yield(YieldContext &, uint64_t data = 0, uint16_t cmd = 0);

  Coroutine(
      const MainFunction &func,
      framework::EventLoop *loop = framework::EventLoop::GetCurrentEventLoop(),
      size_t stack_size = default_stack_size);
  ~Coroutine();

  // only one thread can hold this coroutine,
  // Coroutine instance should be unique
  Coroutine(const Coroutine &) = delete;
  Coroutine &operator=(const Coroutine &) = delete;

  bool finished() const { return finished_; }
  size_t stack_size() const { return stack_size_; }

  void Schedule() override;

  ResumeType Resume(uint64_t data = 0, uint16_t cmd = 0);

 private:
  static uint64_t coroutine_id_;

  size_t stack_size_;
  bool first_call_;
  bool finished_;
  MainFunction main_;
  YieldContext *yield_context_;
};

// 1. 'YieldContext' store beside stack
// 2. when platform use decending stack, struct locate at top of the stack
//    when platform use ascending stack, struct locate at bottom of the stack
struct YieldContext {
  void *caller_sp;
  void *self_sp;
  uint64_t data;
  Coroutine::MainFunction *main;
  // range bit 0~15
  // bit 0: command terminate
  // bit 15: flag finished
  uint64_t cmd;
  char stack_base[0];
};

namespace internal {

typedef void wrapper_type(YieldContext *);

extern "C" void ndsl_coroutine_wrapper_function(YieldContext *);
// Caution:
// 1. User don't need to worry about whether your platform using ascending stack
//    or descending stack
// 2. should pass real stack size
extern "C" void ndsl_start_coroutine(YieldContext *, wrapper_type wrapper,
                                     void *stack_base, size_t real_stack_size);
extern "C" void ndsl_swap_to(void *caller_sp_storage, void *target_sp);

}  // namespace internal

}  // namespace coroutine
}  // namespace ndsl

#endif