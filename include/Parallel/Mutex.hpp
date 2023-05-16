#pragma once
#include <Parallel/LockGuard.hpp>
#include <memory>
#include <pthread.h>

namespace proxy {
class Mutex {
private:
  pthread_mutex_t *MutexHandle;
  bool _IsLocked = false;

public:
  Mutex();
  Mutex(pthread_mutex_t *Handle);

  operator pthread_mutex_t *();

  void Lock(bool CheckInterrupt = true);
  void Unlock(bool CheckInterrupt = true);
  pthread_mutex_t *Get();
  ~Mutex();
};

using MutexPtr = std::shared_ptr<Mutex>;

class MutexLocker : public Locker<Mutex> {
public:
  MutexLocker(Mutex *M) : Locker<Mutex>(M) {}
  virtual void Lock() { _Lock->Lock(_CheckInterrupt); }
  virtual void Unlock() { _Lock->Unlock(_CheckInterrupt); }
};
} // namespace proxy
