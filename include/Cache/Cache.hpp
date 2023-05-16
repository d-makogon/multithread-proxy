#pragma once
#include <Cache/CacheListener.hpp>
#include <Cache/CacheRecord.hpp>
#include <Parallel/ReadWriteLock.hpp>
#include <set>
#include <unordered_map>

namespace proxy {
class Cache {
private:
  ReadWriteLock RecordsLock;
  ReadWriteLock URIListenersLock;
  std::unordered_map<std::string, CacheRecord *> Records;
  std::unordered_map<std::string, std::set<CacheListener *>> URIListeners;

public:
  Cache() = default;

  void PutRecord(const std::string &URI, CacheRecord *Record);
  void AddListener(const std::string &URI, CacheListener *Listener);
  void RemoveListener(const std::string &URI, CacheListener *Listener);
  bool HasRecord(const std::string &URI);
  CacheRecord *TryGetRecord(const std::string &URI);
  CacheRecord *GetRecord(const std::string &URI);

  ~Cache();
};
} // namespace proxy
