#include <Common/Globals.hpp>
#include <Logging/Logger.hpp>
#include <Net/RemoteHandler.hpp>
#include <Net/Server.hpp>
#include <Parallel/LockGuard.hpp>

namespace proxy {
Server::Server(uint16_t Port)
    : ServerTasksSemaphore(0), Poll(std::make_unique<Poller>()),
      SrvCache(std::make_unique<Cache>()) {
  SrvSock = new ServerSocket(Port);
  SrvHandler = new ServerHandler(this);
}

ServerSocket *Server::GetServerSocket() { return SrvSock; }

Cache *Server::GetCache() { return SrvCache.get(); }

void Server::Terminate() {
  LockGuard<WriteLocker> G(&IsTerminatedLock);
  _IsTerminated = true;
}

bool Server::IsTerminated() {
  LockGuard<ReadLocker> G(&IsTerminatedLock);
  return _IsTerminated;
}

void Server::AddCacheListener(CacheListenerInfo CLI) {
  LockGuard<MutexLocker> G(&CacheMutex);
  if (!SrvCache->HasRecord(CLI.CacheAddress)) {
    Log::DefaultLogger.LogDebug("No cache record found, connecting to ",
                                CLI.RemoteHostName, ":", CLI.RemotePort);
    auto *Handler = new RemoteHandler(this, CLI.CacheAddress, CLI.RequestBytes);
    Handler->ConnectTo(CLI.RemoteHostName, CLI.RemotePort);
    Handler->Start();
  }
  SrvCache->AddListener(CLI.CacheAddress, CLI.Listener);
}

void Server::RegisterHandler(PollHandlerBase *HB) {
  LockGuard<MutexLocker> G(&HandlersMutex);
  Handlers.insert(HB);
}

void Server::RemoveHandler(PollHandlerBase *HB) {
  if (!HB)
    return;
  LockGuard<MutexLocker> G(&HandlersMutex);
  auto It = std::find(Handlers.begin(), Handlers.end(), HB);
  Handlers.erase(It);
  if (HB == SrvHandler)
    Terminate();
  delete HB;
}

void Server::EraseDeadHandlers() {
  LockGuard<MutexLocker> G(&DeadHandlersMutex);
  for (auto *HB : DeadHandlers)
    RemoveHandler(HB);
  DeadHandlers.clear();
}

void Server::MarkDeadHandler(PollHandlerBase *HB) {
  if (!HB)
    return;
  {
    LockGuard<MutexLocker> G(&DeadHandlersMutex);
    DeadHandlers.insert(HB);
  }
  ServerTasksSemaphore.Release();
}

void Server::TerminateTimedOutHandlers() {
  auto Now = SocketBase::ClockT::now();
  using FSec = std::chrono::duration<float>;
  LockGuard<MutexLocker> G(&HandlersMutex);
  for (auto *HB : Handlers) {
    if (HB == SrvHandler)
      continue;
    auto Time = HB->GetLastIOTimePoint();
    if (!Time.has_value())
      continue;
    FSec TimePassed = Now - *Time;
    if (TimePassed.count() >= Globals::ClientTimeoutSec) {
      Log::DefaultLogger.LogInfo(
          "Socket #", HB->GetSocket()->GetFD(),
          " has been inactive for too long, disconnecting");
      HB->Terminate();
    }
  }
}

static Server *ServerPtr = nullptr;

extern "C" {
static void OnSignal(int) {
  Log::DefaultLogger.LogDebug("OnSignal");
  if (ServerPtr)
    ServerPtr->Terminate();
}
}

void Server::StartImpl() {
  // ThisThread::DisableInterruption();

  ServerPtr = this;

  struct sigaction OnSignalAction;
  memset(&OnSignalAction, 0, sizeof(OnSignalAction));
  OnSignalAction.sa_handler = &OnSignal;

  if (0 != sigaction(SIGINT, &OnSignalAction, NULL))
    Exception::ThrowSystemError("sigaction()");

  if (0 != sigaction(SIGTERM, &OnSignalAction, NULL))
    Exception::ThrowSystemError("sigaction()");

  Utils::UnlockInterruptionSignals();

  Log::DefaultLogger.LogInfo("Listening at port ", SrvSock->GetPort());

  RegisterHandler(SrvHandler);
  SrvHandler->Start();

  while (!IsTerminated()) {
    try {
      ServerTasksSemaphore.TimedAcquire(Globals::ClientTimeoutMSec);
      EraseDeadHandlers();
      TerminateTimedOutHandlers();
    } catch (const std::system_error &E) {
      if (IsTerminated())
        break;
      Log::DefaultLogger.LogFatal("[Server]: ", E.what());
      Terminate();
    }
  }
}

void Server::Start() {
  try {
    StartImpl();
  } catch (std::system_error &E) {
    Log::DefaultLogger.LogFatal(E.what());
  }
}

Server::~Server() {
  LockGuard<MutexLocker> G(&HandlersMutex, false);
  for (auto *HB : Handlers)
    delete HB;
}
} // namespace proxy
