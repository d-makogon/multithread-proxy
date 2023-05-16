#pragma once
#include <Net/RemoteHandler.hpp>
#include <vector>

namespace proxy {
class RemoteHandler;

class EndToEndHandlerBase {
public:
  virtual void HandleRemoteEndInput(RemoteHandler *RemHandler,
                                    std::vector<char> *Bytes) = 0;
  virtual ~EndToEndHandlerBase() = default;
};
} // namespace proxy
