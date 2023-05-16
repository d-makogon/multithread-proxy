#include <Common/ProxyException.hpp>
#include <Parallel/Condition.hpp>
#include <Parallel/Thread.hpp>
#include <pthread.h>

namespace proxy {
Condition::Condition() {
  CondHandle = new pthread_cond_t();
  int Status = pthread_cond_init(CondHandle, nullptr);
  ThisThread::InterruptionPoint();
  if (Status != 0)
    Exception::ThrowSystemError(Status, "pthread_cond_init");
}

Condition::Condition(pthread_cond_t *Handle) {
  if (!Handle)
    throw std::runtime_error("Null handle");
  CondHandle = Handle;
}

Condition::operator pthread_cond_t *() { return CondHandle; }

void Condition::Wait(Mutex &M) {
  int Status = pthread_cond_wait(CondHandle, M);
  ThisThread::InterruptionPoint();
  if (Status != 0)
    Exception::ThrowSystemError(Status, "pthread_cond_wait");
}

void Condition::Notify() {
  int Status = pthread_cond_signal(CondHandle);
  ThisThread::InterruptionPoint();
  if (Status != 0)
    Exception::ThrowSystemError(Status, "pthread_cond_signal");
}

void Condition::NotifyAll() {
  int Status = pthread_cond_broadcast(CondHandle);
  ThisThread::InterruptionPoint();
  if (Status != 0)
    Exception::ThrowSystemError(Status, "pthread_cond_broadcast");
}

Condition::~Condition() {
  pthread_cond_destroy(CondHandle);
  delete CondHandle;
}
} // namespace proxy
