#pragma once
#include <utility>

namespace proxy {
template <class Fp, class A0, class... Args>
inline auto invoke(Fp &&f, A0 &&a0, Args &&... args)
    -> decltype((std::forward<A0>(a0).*f)(std::forward<Args>(args)...)) {
  return (std::forward<A0>(a0).*f)(std::forward<Args>(args)...);
}
template <class R, class Fp, class A0, class... Args>
inline auto invoke(Fp &&f, A0 &&a0, Args &&... args)
    -> decltype((std::forward<A0>(a0).*f)(std::forward<Args>(args)...)) {
  return (std::forward<A0>(a0).*f)(std::forward<Args>(args)...);
}

template <class Fp, class A0, class... Args>
inline auto invoke(Fp &&f, A0 &&a0, Args &&... args)
    -> decltype(((*std::forward<A0>(a0)).*f)(std::forward<Args>(args)...)) {
  return ((*std::forward<A0>(a0)).*f)(std::forward<Args>(args)...);
}
template <class R, class Fp, class A0, class... Args>
inline auto invoke(Fp &&f, A0 &&a0, Args &&... args)
    -> decltype(((*std::forward<A0>(a0)).*f)(std::forward<Args>(args)...)) {
  return ((*std::forward<A0>(a0)).*f)(std::forward<Args>(args)...);
}

template <class Fp, class A0>
inline auto invoke(Fp &&f, A0 &&a0) -> decltype(std::forward<A0>(a0).*f) {
  return std::forward<A0>(a0).*f;
}

template <class Fp, class A0>
inline auto invoke(Fp &&f, A0 &&a0) -> decltype((*std::forward<A0>(a0)).*f) {
  return (*std::forward<A0>(a0)).*f;
}

template <class R, class Fp, class A0>
inline auto invoke(Fp &&f, A0 &&a0) -> decltype(std::forward<A0>(a0).*f) {
  return std::forward<A0>(a0).*f;
}

template <class R, class Fp, class A0>
inline auto invoke(Fp &&f, A0 &&a0) -> decltype((*std::forward<A0>(a0)).*f) {
  return (*std::forward<A0>(a0)).*f;
}

template <class R, class Fp, class... Args>
inline auto invoke(Fp &&f, Args &&... args)
    -> decltype(std::forward<Fp>(f)(std::forward<Args>(args)...)) {
  return std::forward<Fp>(f)(std::forward<Args>(args)...);
}
template <class Fp, class... Args>
inline auto invoke(Fp &&f, Args &&... args)
    -> decltype(std::forward<Fp>(f)(std::forward<Args>(args)...)) {
  return std::forward<Fp>(f)(std::forward<Args>(args)...);
}
} // namespace proxy
