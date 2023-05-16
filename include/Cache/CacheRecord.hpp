#pragma once
#include <Cache/CacheBlock.hpp>
#include <Cache/CacheListener.hpp>
#include <Parallel/Mutex.hpp>
#include <deque>
#include <set>
#include <string>

namespace proxy {
class CacheListener;

class CacheRecord {
private:
  std::string Address;
  std::deque<CacheBlock *> Blocks;
  std::set<CacheListener *> Listeners;
  std::size_t TotalSize = 0;
  Mutex BlocksMutex;
  Mutex ListenersMutex;
  Mutex TotalSizeMutex;
  Mutex IsFinishedMutex;
  Mutex IsCompleteMutex;
  bool _IsFinished = false;
  bool _IsComplete = false;

  void NotifyRecordUpdate();

public:
  explicit CacheRecord(std::string Address) : Address(std::move(Address)) {}

  void AppendBlock(CacheBlock *Block);
  void AddListener(CacheListener *Listener);
  void RemoveListener(CacheListener *Listener);
  CacheBlock *GetBlock(std::size_t Idx);
  std::size_t GetNumBlocks();
  bool HasBlock(std::size_t Idx);
  void Finish();
  bool IsFinished();
  void SetComplete(bool Complete);
  bool IsComplete();
  std::size_t GetTotalSize();

  ~CacheRecord();
};
} // namespace proxy
