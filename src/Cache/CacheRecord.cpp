#include <Cache/CacheRecord.hpp>
#include <Logging/Logger.hpp>
#include <Parallel/LockGuard.hpp>
#include <algorithm>

namespace proxy {
void CacheRecord::NotifyRecordUpdate() {
  LockGuard<MutexLocker> G(&ListenersMutex);
  for (auto *L : Listeners)
    L->OnCacheRecordUpdate(this);
}

void CacheRecord::AppendBlock(CacheBlock *Block) {
  {
    LockGuard<MutexLocker> G(&BlocksMutex);
    Blocks.push_back(Block);
  }
  {
    LockGuard<MutexLocker> G(&TotalSizeMutex);
    TotalSize += Block->GetBytes().size();
  }
  NotifyRecordUpdate();
}

void CacheRecord::SetComplete(bool Complete) {
  LockGuard<MutexLocker> G(&IsCompleteMutex);
  _IsComplete = Complete;
}

bool CacheRecord::IsComplete() {
  LockGuard<MutexLocker> G(&IsCompleteMutex);
  return _IsComplete;
}

bool CacheRecord::IsFinished() {
  LockGuard<MutexLocker> G(&IsFinishedMutex);
  return _IsFinished;
}

std::size_t CacheRecord::GetTotalSize() {
  LockGuard<MutexLocker> G(&TotalSizeMutex);
  return TotalSize;
}

std::size_t CacheRecord::GetNumBlocks() {
  LockGuard<MutexLocker> G(&BlocksMutex);
  return Blocks.size();
}

void CacheRecord::AddListener(CacheListener *Listener) {
  {
    LockGuard<MutexLocker> G(&ListenersMutex);
    Listeners.insert(Listener);
  }
  Listener->OnCacheRecordUpdate(this);
}

void CacheRecord::RemoveListener(CacheListener *Listener) {
  LockGuard<MutexLocker> G(&ListenersMutex);
  auto It = std::find(Listeners.begin(), Listeners.end(), Listener);
  Listeners.erase(It);
}

CacheBlock *CacheRecord::GetBlock(std::size_t Idx) {
  LockGuard<MutexLocker> G(&BlocksMutex);
  return Blocks.size() > Idx ? Blocks[Idx] : nullptr;
}

bool CacheRecord::HasBlock(std::size_t Idx) {
  LockGuard<MutexLocker> G(&BlocksMutex);
  return Blocks.size() > Idx;
}

void CacheRecord::Finish() {
  LockGuard<MutexLocker> G(&BlocksMutex);
  if (Blocks.size() > 0) {
    (*Blocks.rbegin())->SetFinal(true);
    LockGuard<MutexLocker> G(&IsFinishedMutex);
    _IsFinished = true;
  }
}

CacheRecord::~CacheRecord() {
  LockGuard<MutexLocker> G(&BlocksMutex, false);
  for (auto *B : Blocks)
    delete B;
}
} // namespace proxy
