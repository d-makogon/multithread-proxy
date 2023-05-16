#pragma once
#include <vector>
#include <Parallel/Mutex.hpp>

namespace proxy {
class CacheBlock {
private:
  std::vector<char> Bytes;
  Mutex IsFinalMutex;
  bool _IsFinal = false;

public:
  explicit CacheBlock(std::size_t Capacity = 0) : Bytes() {Bytes.reserve(Capacity);}

  std::vector<char> &GetBytes();
  const std::vector<char> &GetBytes() const;
  void SetFinal(bool Final);
  bool IsFinal();

  ~CacheBlock() = default;
};
} // namespace proxy
