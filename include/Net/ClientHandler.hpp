#pragma once
#include <Cache/CacheListener.hpp>
#include <Cache/CacheRecord.hpp>
#include <Common/Globals.hpp>
#include <Functional/Function.hpp>
#include <Net/EndToEndHandlerBase.hpp>
#include <Net/PollHandlerBase.hpp>
#include <Net/RemoteHandler.hpp>
#include <Net/Server.hpp>
#include <Net/Socket.hpp>
#include <Parallel/Mutex.hpp>
#include <Parallel/Semaphore.hpp>
#include <Parallel/Thread.hpp>
#include <httpparser/request.h>
#include <httpparser/response.h>
#include <memory>
#include <optional>
#include <vector>

namespace proxy {
class ClientHandler : public PollHandlerBase,
                      public EndToEndHandlerBase,
                      public CacheListener {
private:
  Thread ClientThread;
  Mutex CacheEventMutex;
  Semaphore CacheEventSemaphore;

  Mutex IsTerminatedMutex;
  bool _IsTerminated = false;

  Socket *ClientSock = nullptr;
  int SockFD;
  Server *Srv = nullptr;
  Poller Poll;
  httpparser::Request ClientRequest;
  std::vector<char> RequestBytes;
  bool RequestFinished = false;

  bool IsEndToEnd = false;
  RemoteHandler *EndToEndHandler = nullptr;
  std::vector<char> ResponseBuffer;
  bool ReadHeader = false;
  httpparser::Response EndToEndResponse;
  std::vector<char> *RemoteInput = nullptr;

  std::string RemoteHostName;
  uint16_t RemoteHostPort;
  std::string CacheAddress;

  CacheRecord *ResponseCacheRecord = nullptr;
  std::size_t CurBlockIdx = 0;
  std::size_t CurBlockPos = 0;
  std::size_t CacheBlocksNum = 0;
  std::size_t PrevCacheBlocksNum = 0;

  void ClientRoutine();

  void SendRecordFromCache();
  void HandleWriteEvents();

  void HandleClientInput();
  void HandleEndToEndWrite();

  void WaitForCacheRecordUpdate(std::size_t LastBlocksNum);

  bool IsTerminated();
  void Finish();

public:
  ClientHandler(Server *Srv, Socket *ClientSock)
      : Srv(Srv), ClientSock(ClientSock), CacheEventSemaphore(0),
        ClientThread(Function(&ClientHandler::ClientRoutine, this)) {
    SockFD = ClientSock->GetFD();
    ResponseBuffer.reserve(Globals::DefaultResponseBufferSize);
  }

  void Handle(Poller *P, PollClient *Client) override;
  void Start() override;

  void HandleRemoteEndInput(RemoteHandler *RemHandler,
                            std::vector<char> *Bytes) override;

  void OnCacheRecordUpdate(CacheRecord *Record) override;
  void Terminate() override;
  SocketBase *GetSocket() override;
  std::optional<SocketBase::TimePointT> GetLastIOTimePoint() override;

  virtual ~ClientHandler();
};
} // namespace proxy
