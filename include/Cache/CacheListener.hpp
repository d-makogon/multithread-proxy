#pragma once
#include <Cache/CacheRecord.hpp>

namespace proxy {
class CacheRecord;

class CacheListener {
public:
  virtual void OnCacheRecordUpdate(CacheRecord *Record) = 0;
  virtual ~CacheListener() = default;
};
} // namespace proxy
