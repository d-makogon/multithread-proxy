#pragma once
#include <Parallel/LockGuard.hpp>
#include <Parallel/ReadWriteLock.hpp>
#include <chrono>

namespace proxy {
class SocketBase {
public:
  using ClockT = std::chrono::system_clock;
  using TimePointT = ClockT::time_point;

  SocketBase(int Fd = -1) : Fd(Fd) { LastIOTimePoint = ClockT::now(); }

  virtual void SetFd(int Fd) { this->Fd = Fd; }

  virtual int GetFD() { return Fd; }

  virtual void SetNonBlocking(bool NonBlock);
  virtual bool IsNonBlocking();
  virtual ~SocketBase() = default;
  virtual TimePointT GetLastIOTimePoint();
  virtual void UpdateLastIOTimePoint();

protected:
  int Fd;

private:
  bool _IsNonBlocking;
  ReadWriteLock LastIOTimeLock;
  TimePointT LastIOTimePoint;
};
} // namespace proxy
