#pragma once

#include <Logging/Logger.hpp>
#include <Parallel/Thread.hpp>
#include <vector>

namespace proxy {
class ThreadPool {
private:
  std::vector<Thread> Threads;
  size_t InitialThreadsNum;

public:
  ThreadPool(size_t InitialThreadsNum);
  ~ThreadPool();
};
} // namespace proxy
