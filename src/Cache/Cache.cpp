#include <Cache/Cache.hpp>
#include <Parallel/LockGuard.hpp>
#include <algorithm>

namespace proxy {
void Cache::PutRecord(const std::string &URI, CacheRecord *Record) {
  {
    LockGuard<WriteLocker> G(&RecordsLock);
    Records[URI] = Record;
  }

  LockGuard<ReadLocker> G(&URIListenersLock);
  auto ListenersIt = URIListeners.find(URI);
  if (ListenersIt == URIListeners.end())
    return;

  for (CacheListener *CL : ListenersIt->second)
    Record->AddListener(CL);
}

CacheRecord *Cache::GetRecord(const std::string &URI) {
  {
    LockGuard<ReadLocker> G(&RecordsLock);
    auto It = Records.find(URI);
    if (It != Records.end())
      return It->second;
  }
  auto *CR = new CacheRecord(URI);
  PutRecord(URI, CR);
  return CR;
}

bool Cache::HasRecord(const std::string &URI) {
  LockGuard<ReadLocker> G(&RecordsLock);
  return Records.find(URI) != Records.end();
}

CacheRecord *Cache::TryGetRecord(const std::string &URI) {
  LockGuard<ReadLocker> G(&RecordsLock);
  auto It = Records.find(URI);
  if (It != Records.end())
    return It->second;
  return nullptr;
}

void Cache::AddListener(const std::string &URI, CacheListener *Listener) {
  {
    LockGuard<WriteLocker> G(&URIListenersLock);
    auto It = URIListeners.find(URI);
    if (It == URIListeners.end())
      URIListeners[URI] = {Listener};
    else
      It->second.insert(Listener);
  }

  if (auto *Record = TryGetRecord(URI))
    Record->AddListener(Listener);
}

void Cache::RemoveListener(const std::string &URI, CacheListener *Listener) {
  {
    LockGuard<ReadLocker> G(&URIListenersLock);
    auto ListenersIt = URIListeners.find(URI);
    if (ListenersIt == URIListeners.end())
      return;
    auto &Vec = ListenersIt->second;
    auto It = std::find(Vec.begin(), Vec.end(), Listener);
    if (It != Vec.end())
      Vec.erase(It);
  }

  if (auto *Record = TryGetRecord(URI))
    Record->RemoveListener(Listener);
}

Cache::~Cache() {
  LockGuard<WriteLocker> G(&RecordsLock, false);
  for (auto &It : Records)
    delete It.second;
}
} // namespace proxy
