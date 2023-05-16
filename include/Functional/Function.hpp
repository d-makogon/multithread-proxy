#pragma once
#include <Functional/Invoke.hpp>
#include <Functional/TupleIndices.hpp>
#include <functional>
#include <tuple>

namespace proxy {
template <typename F, class... ArgTypes> class Function {
private:
  std::tuple<typename std::decay<F>::type,
             typename std::decay<ArgTypes>::type...>
      Func;

  template <std::size_t... Indices> void RunImpl(TupleIndices<Indices...>) {
    proxy::invoke(std::move(std::get<0>(Func)),
                  std::move(std::get<Indices>(Func))...);
  }

public:
  explicit Function(F &&Func, ArgTypes &&... Args)
      : Func(std::forward<F>(Func), std::forward<ArgTypes>(Args)...) {}

  void Run() {
    using IndexType = typename MakeTupleIndices<
        std::tuple_size<std::tuple<F, ArgTypes...>>::value, 1>::type;
    RunImpl(IndexType());
  }

  virtual ~Function() = default;
};
} // namespace proxy
