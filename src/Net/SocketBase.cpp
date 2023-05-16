#include <Common/ProxyException.hpp>
#include <Logging/Logger.hpp>
#include <Net/SocketBase.hpp>
#include <Parallel/LockGuard.hpp>
#include <chrono>
#include <fcntl.h>

namespace proxy {
void SocketBase::UpdateLastIOTimePoint() {
  LockGuard<WriteLocker> G(&LastIOTimeLock);
  LastIOTimePoint = ClockT::now();
}

SocketBase::TimePointT SocketBase::GetLastIOTimePoint() {
  return LastIOTimePoint;
}

bool SocketBase::IsNonBlocking() { return _IsNonBlocking; }

void SocketBase::SetNonBlocking(bool NonBlock) {
  int Flag = O_NONBLOCK;
  if (!Flag)
    Flag = ~Flag;
  if (fcntl(Fd, F_SETFL, fcntl(Fd, F_GETFL) | Flag) < 0)
    Exception::ThrowSystemError("fcntl()");
  Log::DefaultLogger.LogDebug("Socket #", Fd, " set to ",
                              (NonBlock ? " non-" : ""), "blocking");
  _IsNonBlocking = NonBlock;
}
} // namespace proxy
