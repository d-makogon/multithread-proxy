#include <Cache/CacheBlock.hpp>
#include <Parallel/LockGuard.hpp>

namespace proxy {
std::vector<char> &CacheBlock::GetBytes() { return Bytes; }

const std::vector<char> &CacheBlock::GetBytes() const { return Bytes; }

void CacheBlock::SetFinal(bool Final) {
  LockGuard<MutexLocker> G(&IsFinalMutex);
  _IsFinal = Final;
}

bool CacheBlock::IsFinal() {
  LockGuard<MutexLocker> G(&IsFinalMutex);
  return _IsFinal;
}
} // namespace proxy
