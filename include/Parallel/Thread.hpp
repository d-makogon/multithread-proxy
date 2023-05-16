#pragma once
#include <Common/ProxyException.hpp>
#include <Functional/Function.hpp>
#include <Parallel/ThreadData.hpp>
#include <iomanip>
#include <memory>
#include <ostream>
#include <unistd.h>

#define THREAD_INTERRUPT_SIG SIGUSR1

namespace proxy {
class Thread {
private:
  ThreadDataPtr OwnThreadData;

public:
  Thread() {}

  Thread(const Thread &T) = delete;

  Thread(Thread &&T);

  template <class F>
  explicit Thread(F &&Func)
      : OwnThreadData(MakeThreadData(std::forward<F>(Func))) {}

  Thread &operator=(const Thread &Other) = delete;

  Thread &operator=(Thread &&Other) {
    if (Joinable())
      std::terminate();
    OwnThreadData = Other.OwnThreadData;
    Other.OwnThreadData.reset();
    return *this;
  }

  void StartThread();
  bool Joinable() const;
  void Join();
  void Detach();
  pthread_t GetHandle();
  void Interrupt();
  bool InterruptRequested();
  void SetInterruptSignalHandler();

  class Id;
  Id GetId() const;

  ~Thread() {
    if (Joinable())
      std::terminate();
  };

private:
  bool StartThreadNoExcept();
  bool StartThreadNoExcept(int &Status);
  bool JoinNoExcept();
  bool JoinNoExcept(int &Status);

  template <class F> static inline ThreadDataPtr MakeThreadData(F &&Func) {
    return ThreadDataPtr(new ThreadData<F>(std::forward<F>(Func)));
  }
};

namespace ThisThread {
extern void InterruptionPoint();
void EnableInterruption();
void DisableInterruption();
void BlockInterruptionSignals();
inline Thread::Id GetId();
inline void Sleep(useconds_t Useconds) {
  usleep(Useconds);
  ThisThread::InterruptionPoint();
}
} // namespace ThisThread

class Thread::Id {
private:
  pthread_t ThreadHandle;

  friend class Thread;
  friend Id ThisThread::GetId();

public:
  Id() : ThreadHandle(0) {}
  Id(pthread_t ThreadHandle) : ThreadHandle(ThreadHandle) {}

  operator pthread_t() { return ThreadHandle; }

  bool operator==(const Id &y) const { return ThreadHandle == y.ThreadHandle; }

  bool operator!=(const Id &y) const { return ThreadHandle != y.ThreadHandle; }

  bool operator<(const Id &y) const { return ThreadHandle < y.ThreadHandle; }

  bool operator>(const Id &y) const { return y.ThreadHandle < ThreadHandle; }

  bool operator<=(const Id &y) const {
    return !(y.ThreadHandle < ThreadHandle);
  }

  bool operator>=(const Id &y) const {
    return !(ThreadHandle < y.ThreadHandle);
  }

  template <class charT, class traits>
  friend std::basic_ostream<charT, traits> &
  operator<<(std::basic_ostream<charT, traits> &os, const Id &Id) {
    if (!Id.ThreadHandle)
      return os << "[Unknown Thread]";
    std::ios_base::fmtflags OldFlags(os.flags());
    os << std::hex << Id.ThreadHandle;
    os.flags(OldFlags);
    return os;
  }
};

namespace ThisThread {
inline Thread::Id GetId() { return Thread::Id(pthread_self()); }
} // namespace ThisThread
} // namespace proxy
