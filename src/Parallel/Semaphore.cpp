#include <Common/ProxyException.hpp>
#include <Parallel/Semaphore.hpp>
#include <Parallel/Thread.hpp>
#include <cerrno>
#include <semaphore.h>
#include <time.h>

namespace proxy {
Semaphore::Semaphore(unsigned InitialValue)
    : SemHandle(std::make_unique<sem_t>()) {
  if (0 != sem_init(SemHandle.get(), 0, InitialValue))
    Exception::ThrowSystemError("sem_init()");
}

void Semaphore::Acquire(bool CheckInterrupt) {
  int Status = sem_wait(SemHandle.get());
  if (CheckInterrupt)
    ThisThread::InterruptionPoint();
  if (Status != 0)
    Exception::ThrowSystemError("sem_wait()");
}

void Semaphore::TimedAcquire(std::size_t Milliseconds, bool CheckInterrupt) {
  struct timespec TS;
  if (0 != clock_gettime(CLOCK_REALTIME, &TS)) {
    if (CheckInterrupt)
      ThisThread::InterruptionPoint();
    Exception::ThrowSystemError("clock_gettime()");
  }

  TS.tv_nsec += Milliseconds * 1000000;
  TS.tv_sec += TS.tv_nsec / 1000000000;
  TS.tv_nsec %= 1000000000;

  int Status = sem_timedwait(SemHandle.get(), &TS);
  if (CheckInterrupt)
    ThisThread::InterruptionPoint();
  if (Status != 0)
    Exception::ThrowSystemError("sem_timedwait()");
}

void Semaphore::Release(bool CheckInterrupt) {
  int Status = sem_post(SemHandle.get());
  if (CheckInterrupt)
    ThisThread::InterruptionPoint();
  if (Status != 0)
    Exception::ThrowSystemError("sem_wait()");
}

Semaphore::~Semaphore() { sem_destroy(SemHandle.get()); }
} // namespace proxy
