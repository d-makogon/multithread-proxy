#include <Common/ProxyException.hpp>
#include <Parallel/Mutex.hpp>
#include <Parallel/Thread.hpp>

namespace proxy {
Mutex::Mutex() {
  MutexHandle = new pthread_mutex_t();
  int Status = pthread_mutex_init(MutexHandle, nullptr);
  if (Status != 0)
    Exception::ThrowSystemError(Status, "pthread_mutex_init");
}

Mutex::Mutex(pthread_mutex_t *Handle) {
  if (!Handle)
    throw std::runtime_error("Null handle");
  MutexHandle = Handle;
}

Mutex::operator pthread_mutex_t *() { return MutexHandle; }

void Mutex::Lock(bool CheckInterrupt) {
  int Status = pthread_mutex_lock(MutexHandle);
  _IsLocked = true;
  // if (CheckInterrupt)
  //   ThisThread::InterruptionPoint();
  if (Status != 0)
    Exception::ThrowSystemError(Status, "pthread_mutex_lock");
}

void Mutex::Unlock(bool CheckInterrupt) {
  _IsLocked = false;
  int Status = pthread_mutex_unlock(MutexHandle);
  // if (CheckInterrupt)
  //   ThisThread::InterruptionPoint();
  if (Status != 0)
    Exception::ThrowSystemError(Status, "pthread_mutex_unlock");
}

pthread_mutex_t *Mutex::Get() { return MutexHandle; }

Mutex::~Mutex() {
  if (_IsLocked)
    pthread_mutex_unlock(MutexHandle);
  pthread_mutex_destroy(MutexHandle);
  delete MutexHandle;
}
} // namespace proxy
