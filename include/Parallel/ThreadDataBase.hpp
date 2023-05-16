#pragma once
#include <Parallel/Condition.hpp>
#include <Parallel/Mutex.hpp>
#include <memory>
#include <pthread.h>
#include <signal.h>

namespace proxy {

struct ThreadDataBase;
using ThreadDataPtr = std::shared_ptr<ThreadDataBase>;

struct ThreadDataBase {
  ThreadDataPtr Self;
  MutexPtr DataMutex;
  ConditionPtr DoneCondition;
  pthread_t ThreadHandle = 0;
  bool Done = false;
  bool JoinStarted = false;
  bool Joined = false;
  bool InterruptEnabled = true;
  bool InterruptRequested = false;

  ThreadDataBase() : DataMutex(new Mutex()), DoneCondition(new Condition()) {}

  virtual void Run() = 0;
  virtual void BlockInterruptionSignals();

  virtual ~ThreadDataBase() = default;
};
} // namespace proxy
