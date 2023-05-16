#pragma once

#include <Cache/Cache.hpp>
#include <Cache/CacheListener.hpp>
#include <Net/PollHandlerBase.hpp>
#include <Net/Poller.hpp>
#include <Net/ServerHandler.hpp>
#include <Net/ServerSocket.hpp>
#include <Net/SocketBase.hpp>
#include <Parallel/Mutex.hpp>
#include <Parallel/Semaphore.hpp>
#include <Parallel/ReadWriteLock.hpp>
#include <deque>
#include <tuple>
#include <memory>

namespace proxy {

class ServerHandler;

struct CacheListenerInfo {
  std::string CacheAddress;
  std::string RemoteHostName;
  uint16_t RemotePort;
  const std::vector<char> &RequestBytes;
  CacheListener *Listener;
};

class Server {
private:
  ServerSocket *SrvSock = nullptr;
  ServerHandler *SrvHandler = nullptr;
  std::unique_ptr<Cache> SrvCache;
  std::unique_ptr<Poller> Poll;
  ReadWriteLock IsTerminatedLock;
  bool _IsTerminated = false;
  Mutex HandlersMutex;
  Mutex DeadHandlersMutex;
  Semaphore ServerTasksSemaphore;
  Mutex CacheMutex;
  std::set<PollHandlerBase *> Handlers;
  std::set<PollHandlerBase *> DeadHandlers;

  void TerminateTimedOutHandlers();
  void EraseDeadHandlers();
  void StartImpl();

public:
  explicit Server(uint16_t Port);
  void Start();
  ServerSocket *GetServerSocket();
  Cache *GetCache();
  void AddCacheListener(CacheListenerInfo CLI);
  void RegisterHandler(PollHandlerBase *HB);
  void RemoveHandler(PollHandlerBase *HB);
  void MarkDeadHandler(PollHandlerBase *HB);
  bool IsTerminated();
  void Terminate();

  ~Server();
};
} // namespace proxy
