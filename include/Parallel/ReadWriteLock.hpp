#pragma once
#include <Parallel/LockGuard.hpp>
#include <memory>
#include <pthread.h>

namespace proxy {
class ReadWriteLock {
private:
  std::unique_ptr<pthread_rwlock_t> RWHandle;

public:
  ReadWriteLock();
  void ReadLock(bool CheckInterrupt = true);
  void WriteLock(bool CheckInterrupt = true);
  void Unlock(bool CheckInterrupt = true);
  ~ReadWriteLock();
};

class ReadLocker : public Locker<ReadWriteLock> {
public:
  ReadLocker(ReadWriteLock *RW) : Locker<ReadWriteLock>(RW) {}
  virtual void Lock() { _Lock->ReadLock(_CheckInterrupt); }
  virtual void Unlock() { _Lock->Unlock(_CheckInterrupt); }
};

class WriteLocker : public Locker<ReadWriteLock> {
public:
  WriteLocker(ReadWriteLock *RW) : Locker<ReadWriteLock>(RW) {}
  virtual void Lock() { _Lock->WriteLock(_CheckInterrupt); }
  virtual void Unlock() { _Lock->Unlock(_CheckInterrupt); }
};
} // namespace proxy
