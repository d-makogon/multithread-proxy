#pragma once
#include <Parallel/Mutex.hpp>
#include <memory>
#include <pthread.h>

namespace proxy {
class Condition {
private:
  pthread_cond_t *CondHandle;

public:
  Condition();
  Condition(pthread_cond_t *Handle);
  operator pthread_cond_t *();

  void Wait(Mutex &M);
  void Notify();
  void NotifyAll();
  ~Condition();
};

using ConditionPtr = std::shared_ptr<Condition>;
} // namespace proxy
