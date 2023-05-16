#pragma once

namespace proxy {
template <typename LockT> class Locker {
protected:
  bool _CheckInterrupt;
  LockT *_Lock;

public:
  Locker(LockT *_Lock) : _Lock(_Lock) {}
  virtual void SetCheckInterrupt(bool V) { _CheckInterrupt = V; };
  virtual void Lock() = 0;
  virtual void Unlock() = 0;
  virtual ~Locker() = default;
};

template <typename LockerT> class LockGuard {
private:
  bool CheckInterrupt;
  LockerT _Locker;

public:
  template <typename LockT>
  LockGuard(LockT &&L, bool CheckInterrupt = true)
      : _Locker(std::forward<LockT>(L)), CheckInterrupt(CheckInterrupt) {
    _Locker.SetCheckInterrupt(CheckInterrupt);
    Lock();
  }

  void Lock() { _Locker.Lock(); }

  void Unlock() { _Locker.Unlock(); }

  ~LockGuard() {
    _Locker.SetCheckInterrupt(false);
    Unlock();
  }
};
} // namespace proxy
