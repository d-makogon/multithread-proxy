#include <Common/ProxyException.hpp>
#include <Parallel/ReadWriteLock.hpp>
#include <Parallel/Thread.hpp>

namespace proxy {
ReadWriteLock::ReadWriteLock()
    : RWHandle(std::make_unique<pthread_rwlock_t>()) {
  // Use default attributes for rwlocks
  int Status = pthread_rwlock_init(RWHandle.get(), NULL);
  if (Status != 0)
    Exception::ThrowSystemError(Status, "pthread_rwlock_init()");
}

void ReadWriteLock::ReadLock(bool CheckInterrupt) {
  int Status = pthread_rwlock_rdlock(RWHandle.get());
  if (Status != 0)
    Exception::ThrowSystemError(Status, "pthread_rwlock_rdlock()");
}

void ReadWriteLock::WriteLock(bool CheckInterrupt) {
  int Status = pthread_rwlock_wrlock(RWHandle.get());
  if (Status != 0)
    Exception::ThrowSystemError(Status, "pthread_rwlock_wrlock()");
}

void ReadWriteLock::Unlock(bool CheckInterrupt) {
  int Status = pthread_rwlock_unlock(RWHandle.get());
  if (Status != 0)
    Exception::ThrowSystemError(Status, "pthread_rwlock_unlock()");
}

ReadWriteLock::~ReadWriteLock() { pthread_rwlock_destroy(RWHandle.get()); }
} // namespace proxy
