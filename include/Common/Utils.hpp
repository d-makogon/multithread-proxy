#pragma once
#include <Common/ProxyException.hpp>
#include <Logging/Logger.hpp>
#include <cstdint>
#include <cstring>
#include <httpparser/request.h>
#include <iostream>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <type_traits>

namespace proxy {
struct Utils {
  template <typename T>
  static bool
  StrToInt(typename std::enable_if<std::is_integral<T>::value, T>::type &TRef,
           std::string String) {
    T t;
    std::stringstream convert(String);
    if ((convert >> t).fail() || !(convert >> std::ws).eof())
      return false;
    TRef = t;
    return true;
  }

  static std::string EventsToString(short Events) {
    std::stringstream ss;
    const char *Prefix = "";

#define APPEND_EVENT(EventMacro)                                               \
  do {                                                                         \
    if (Events & EventMacro) {                                                 \
      ss << Prefix << #EventMacro;                                             \
      Prefix = " | ";                                                          \
    }                                                                          \
  } while (0)

    APPEND_EVENT(POLLIN);
    APPEND_EVENT(POLLRDNORM);
    APPEND_EVENT(POLLRDBAND);
    APPEND_EVENT(POLLPRI);
    APPEND_EVENT(POLLOUT);
    APPEND_EVENT(POLLWRNORM);
    APPEND_EVENT(POLLWRBAND);
    APPEND_EVENT(POLLERR);
    APPEND_EVENT(POLLHUP);
    APPEND_EVENT(POLLNVAL);
    return ss.str();
  }

  static std::string RequestToString(const httpparser::Request &R) {
    std::stringstream stream;
    stream << R.method << " " << R.uri << " HTTP/" << R.versionMajor << "."
           << R.versionMinor << "\r\n";

    for (auto It = R.headers.begin(); It != R.headers.end(); ++It)
      stream << It->name << ": " << It->value << "\r\n";

    std::string data(R.content.begin(), R.content.end());
    stream << data << "\r\n";
    return stream.str();
  }

  template <class InputIt1, class InputIt2, class OutputIt>
  static OutputIt ConcatRanges(InputIt1 First1, InputIt1 Last1, InputIt2 First2,
                               InputIt2 Last2, OutputIt Out) {
    Out = std::copy(First1, Last1, Out);
    return std::copy(First2, Last2, Out);
  }

  static void BlockInterruptionSignals() {
    sigset_t SigMask;
    sigemptyset(&SigMask);
    FillInterruptionSignals(SigMask);
    int Status = pthread_sigmask(SIG_BLOCK, &SigMask, NULL);
    if (Status != 0)
      Exception::ThrowSystemError(Status);
  }

  static void UnlockInterruptionSignals() {
    sigset_t SigMask;
    sigemptyset(&SigMask);
    FillInterruptionSignals(SigMask);
    int Status = pthread_sigmask(SIG_UNBLOCK, &SigMask, NULL);
    if (Status != 0)
      Exception::ThrowSystemError(Status);
  }

private:
  static void FillInterruptionSignals(sigset_t &SS) {
    sigaddset(&SS, SIGINT);
    sigaddset(&SS, SIGTERM);
  }
};
} // namespace proxy
