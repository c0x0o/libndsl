#include <iostream>
#include <ndsl/coroutine/Coroutine.hpp>

using ndsl::coroutine::Coroutine;
using ndsl::coroutine::YieldContext;

uint64_t Function1(YieldContext &context) {
  uint64_t pushed_data;
  std::cout << "Fun1 started" << std::endl;
  pushed_data = Coroutine::Yield(context, 1);
  std::cout << "Fun1 start again, pushed data is " << pushed_data << std::endl;
  pushed_data = Coroutine::Yield(context, 2);
  std::cout << "Fun1 start again, pushed data is " << pushed_data << std::endl;

  return 0;
}

int main() {
  Coroutine co1(Function1);
  int count = 0;

  std::cout << "Main Thread start, calling co1..." << std::endl;

  while (!co1.finished() && count < 10) {
    uint64_t pulled_data;
    if (!co1.finished()) {
      pulled_data = co1.Resume(1);
      std::cout << "co1 resumed, pulled data is " << pulled_data << std::endl;
    }

    count++;
  }

  if (co1.finished()) {
    std::cout << "co1 ended" << std::endl;
  }

  return 0;
}