#pragma once
#include <Net/PollHandlerBase.hpp>
#include <Net/Poller.hpp>
#include <Net/Server.hpp>
#include <Net/ServerSocket.hpp>
#include <Parallel/Thread.hpp>
#include <Parallel/ReadWriteLock.hpp>
#include <memory>
#include <optional>

namespace proxy {
class Server;
class ServerHandler : public PollHandlerBase {
private:
  Server *Srv = nullptr;
  ServerSocket *Sock = nullptr;
  int SockFD;
  Poller Poll;
  Thread ServerThread;
  ReadWriteLock IsTerminatedLock;
  bool _IsTerminated = false;
  void ServerRoutine();
  void Finish();

public:
  explicit ServerHandler(Server *Server);
  void Handle(Poller *P, PollClient *Client);
  void Terminate() override;
  void Start() override;
  bool IsTerminated();
  SocketBase *GetSocket() override;
  std::optional<SocketBase::TimePointT> GetLastIOTimePoint() override;
  virtual ~ServerHandler();
};
} // namespace proxy
