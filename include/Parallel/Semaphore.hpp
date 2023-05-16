#pragma once
#include <memory>
#include <semaphore.h>

namespace proxy {
class Semaphore {
private:
  std::unique_ptr<sem_t> SemHandle;

public:
  Semaphore(unsigned InitialValue);
  void Acquire(bool CheckInterrupt = true);
  void TimedAcquire(std::size_t Milliseconds, bool CheckInterrupt = true);
  void Release(bool CheckInterrupt = true);
  ~Semaphore();
};
} // namespace proxy
