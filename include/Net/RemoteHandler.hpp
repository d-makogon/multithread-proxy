#pragma once
#include <Functional/Function.hpp>
#include <Net/EndToEndHandlerBase.hpp>
#include <Net/PollHandlerBase.hpp>
#include <Net/Server.hpp>
#include <Net/Socket.hpp>
#include <Net/SocketBase.hpp>
#include <Parallel/Condition.hpp>
#include <Parallel/Mutex.hpp>
#include <Parallel/Thread.hpp>
#include <httpparser/httprequestparser.h>
#include <httpparser/request.h>
#include <memory>
#include <optional>
#include <vector>

namespace proxy {
class EndToEndHandlerBase;

class RemoteHandler : public PollHandlerBase {
public:
  enum class Mode { Cache, EndToEnd };

  RemoteHandler(Server *Srv, std::string RemoteAddress,
                const std::vector<char> &RequestBytes, Mode _Mode = Mode::Cache)
      : Srv(Srv), RemoteAddress(std::move(RemoteAddress)),
        RequestBytes(RequestBytes), _Mode(_Mode),
        RemoteThread(Function(&RemoteHandler::RemoteRoutine, this)) {
    Srv->RegisterHandler(this);
  }

  void ConnectTo(const std::string &Host, uint16_t Port);

  Mode GetRemoteMode() const;
  Socket *GetRemoteSocket();
  void Unregister();
  void Handle(Poller *P, PollClient *Client) override;
  void HandleRemoteInput(const PollClient &Client);
  void Terminate() override;
  void SetEndToEndBuffer(std::vector<char> *EndToEndBuffer);
  void SetEndToEndWriteHandler(EndToEndHandlerBase *EndToEndWriteHandler);
  SocketBase *GetSocket() override;
  std::optional<SocketBase::TimePointT> GetLastIOTimePoint() override;
  void Start() override;

  virtual ~RemoteHandler();

private:
  Thread RemoteThread;

  Server *Srv;
  CacheRecord *CR = nullptr;
  Socket *RemoteSock = nullptr;
  Poller Poll;
  std::string RemoteAddress;
  bool HandledConnect = false;
  Mutex IsTerminatedMutex;
  bool _IsTerminated = false;
  const std::vector<char> &RequestBytes;
  std::vector<char> *EndToEndBuffer;
  EndToEndHandlerBase *EndToEndWriteHandler;
  Mode _Mode;

  void RemoteRoutine();
  bool IsTerminated();
  void Finish();

  void HandleConnect(const PollClient &Client);
  void ReadToCache();
  void ReadEndToEnd();
};
} // namespace proxy
