#include <Common/Globals.hpp>
#include <Logging/Logger.hpp>
#include <Net/Poller.hpp>
#include <Net/RemoteHandler.hpp>
#include <Net/Server.hpp>
#include <Parallel/LockGuard.hpp>
#include <cassert>
#include <memory>
#include <optional>

namespace proxy {
void RemoteHandler::Finish() {
  Terminate();
  Srv->MarkDeadHandler(this);
}

void RemoteHandler::Terminate() {
  {
    LockGuard<MutexLocker> G(&IsTerminatedMutex);
    if (_IsTerminated)
      return;
    _IsTerminated = true;
  }
  if (CR) {
    CR->SetComplete(CR->IsFinished());
    CR->Finish();
  }
}

SocketBase *RemoteHandler::GetSocket() { return RemoteSock; }

std::optional<SocketBase::TimePointT> RemoteHandler::GetLastIOTimePoint() {
  if (RemoteSock)
    return RemoteSock->GetLastIOTimePoint();
  return {};
}

RemoteHandler::Mode RemoteHandler::GetRemoteMode() const { return _Mode; }

void RemoteHandler::SetEndToEndBuffer(std::vector<char> *EndToEndBuffer) {
  this->EndToEndBuffer = EndToEndBuffer;
}

void RemoteHandler::SetEndToEndWriteHandler(
    EndToEndHandlerBase *EndToEndWriteHandler) {
  this->EndToEndWriteHandler = EndToEndWriteHandler;
}

void RemoteHandler::ReadToCache() {
  auto *C = Srv->GetCache();
  CR = C->GetRecord(RemoteAddress);

  std::unique_ptr<CacheBlock> CB;
  try {
    CB.reset(new CacheBlock(Globals::DefaultCacheBlockSize));
  } catch (const std::bad_alloc &BA) {
    Log::DefaultLogger.LogInfo(
        "[Remote #", RemoteSock->GetFD(),
        "] There is insufficient amount of RAM available, stopping "
        "cache downloading");
    Finish();
    return;
  }

  std::size_t ReadBytes = RemoteSock->Read(CB->GetBytes());

  Log::DefaultLogger.LogDebug("[Remote #", RemoteSock->GetFD(), "] Received ",
                              ReadBytes, " bytes");

  if (ReadBytes == 0) {
    Log::DefaultLogger.LogDebug("[Remote #", RemoteSock->GetFD(),
                                "] Client terminated connection");
    CR->Finish();
    Finish();
    return;
  }

  Log::DefaultLogger.LogDebug("[Remote #", RemoteSock->GetFD(),
                              "] Creating new cache block");
  CR->AppendBlock(CB.release());
}

void RemoteHandler::ReadEndToEnd() {
  EndToEndBuffer->clear();
  RemoteSock->ReadAppend(*EndToEndBuffer);
  Log::DefaultLogger.LogInfo("[Remote #", RemoteSock->GetFD(), "] Received ",
                             EndToEndBuffer->size(),
                             " bytes in end-to-end mode");
  EndToEndWriteHandler->HandleRemoteEndInput(this, EndToEndBuffer);
  Unregister();
}

void RemoteHandler::HandleRemoteInput(const PollClient &Client) {
  if (_Mode == Mode::Cache)
    ReadToCache();
  else if (_Mode == Mode::EndToEnd)
    ReadEndToEnd();
}

void RemoteHandler::Unregister() { Poll.Remove(RemoteSock->GetFD(), POLLIN); }

void RemoteHandler::ConnectTo(const std::string &Host, uint16_t Port) {
  RemoteSock = Socket::ConnectTo(Host, Port);
  Poll.Add(RemoteSock->GetFD(), POLLOUT, this);
}

void RemoteHandler::HandleConnect(const PollClient &Client) {
  int SockError;
  socklen_t SockErrorLen = sizeof(SockError);
  int Status = getsockopt(Client.GetFD(), SOL_SOCKET, SO_ERROR, &SockError,
                          &SockErrorLen);
  if (Status < 0)
    Exception::ThrowSystemError("getsockopt()");
  HandledConnect = true;
  Poll.Remove(Client.GetFD(), POLLOUT);
  RemoteSock->Write(RequestBytes);
}

bool RemoteHandler::IsTerminated() {
  LockGuard<MutexLocker> G(&IsTerminatedMutex);
  return _IsTerminated;
}

void RemoteHandler::RemoteRoutine() {
  ThisThread::BlockInterruptionSignals();
  int FD = RemoteSock->GetFD();
  Poll.Add(RemoteSock->GetFD(), POLLIN, this);
  while (!IsTerminated()) {
    Log::DefaultLogger.LogDebug("[Remote #", FD, "] Waiting for events...");
    int NumPolled = Poll.Poll(Globals::ClientTimeoutMSec);
    if (NumPolled == 0) {
      Finish();
      return;
    }

    for (auto &Client : Poll) {
      Log::DefaultLogger.LogDebug(
          "FD ", Client.GetFD(), " received ",
          Utils::EventsToString(Client.GetReceivedEvents()));
      try {
        Handle(&Poll, &Client);
      } catch (std::system_error &E) {
        Log::DefaultLogger.LogFatal("[Remote #", Client.GetFD(),
                                    "]: ", E.what());
        Finish();
      }
      ThisThread::InterruptionPoint();
    }
  }

  Log::DefaultLogger.LogDebug("[Remote #", FD, "] Terminating");
}

void RemoteHandler::Start() { RemoteThread.StartThread(); }

void RemoteHandler::Handle(Poller *P, PollClient *Client) {
  short Events = Client->GetReceivedEvents();
  if (Events & (POLLHUP | POLLNVAL | POLLERR)) {
    Log::DefaultLogger.LogDebug("[Remote #", RemoteSock->GetFD(),
                                "] Remote terminated connection");
    Finish();
    return;
  }

  if (Events & POLLOUT)
    HandleConnect(*Client);

  if (Events & POLLIN)
    HandleRemoteInput(*Client);
}

RemoteHandler::~RemoteHandler() {
  Terminate();
  RemoteThread.Interrupt();
  RemoteThread.Join();
  if (RemoteSock)
    delete RemoteSock;
}
} // namespace proxy
