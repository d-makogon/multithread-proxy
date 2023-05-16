#pragma once
#include <Functional/Function.hpp>
#include <Parallel/ThreadDataBase.hpp>
#include <functional>
#include <tuple>

namespace proxy {
template <typename F> class ThreadData : public ThreadDataBase {
private:
  F Func;

public:
  ThreadData(F &&Func) : Func(std::forward<F>(Func)) {}

  void Run() { Func.Run(); }

  virtual ~ThreadData() = default;
};
} // namespace proxy
