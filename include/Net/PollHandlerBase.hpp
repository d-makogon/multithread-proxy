#pragma once

#include <Net/Poller.hpp>
#include <Net/SocketBase.hpp>
#include <optional>

namespace proxy {

class PollClient;
class Poller;

class PollHandlerBase {
public:
  virtual SocketBase *GetSocket() = 0;
  virtual std::optional<SocketBase::TimePointT> GetLastIOTimePoint() = 0;
  virtual void Terminate() = 0;
  virtual void Start() = 0;
  virtual void Handle(Poller *P, PollClient *Client) = 0;
  virtual ~PollHandlerBase() = default;
};
} // namespace proxy
