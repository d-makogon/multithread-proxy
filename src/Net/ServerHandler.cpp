#include <Net/ClientHandler.hpp>
#include <Net/ServerHandler.hpp>
#include <Parallel/LockGuard.hpp>
#include <cassert>
#include <optional>
#include <poll.h>

namespace proxy {
ServerHandler::ServerHandler(Server *Server)
    : ServerThread(Function(&ServerHandler::ServerRoutine, this)) {
  Srv = Server;
  Sock = Srv->GetServerSocket();
  SockFD = Sock->GetFD();
}

SocketBase *ServerHandler::GetSocket() { return Sock; }

std::optional<SocketBase::TimePointT> ServerHandler::GetLastIOTimePoint() {
  if (Sock)
    return Sock->GetLastIOTimePoint();
  return {};
}

void ServerHandler::Terminate() {
  LockGuard<WriteLocker> G(&IsTerminatedLock, false);
  _IsTerminated = true;
}

bool ServerHandler::IsTerminated() {
  LockGuard<ReadLocker> G(&IsTerminatedLock, false);
  return _IsTerminated;
}

void ServerHandler::Finish() {
  Terminate();
  Srv->MarkDeadHandler(this);
}

void ServerHandler::ServerRoutine() {
  ThisThread::BlockInterruptionSignals();
  Sock->Listen();
  Poll.Add(SockFD, POLLIN, this);
  while (!IsTerminated()) {
    Log::DefaultLogger.LogDebug("Server: Polling...");
    int PolledNum = Poll.Poll(Globals::ClientTimeoutMSec);
    if (PolledNum == 0) {
      Finish();
      break;
    }

    int HandledNum = 0;

    for (auto &Client : Poll) {
      if (HandledNum++ == PolledNum)
        break;
      Log::DefaultLogger.LogDebug(
          "FD ", Client.GetFD(), " received ",
          Utils::EventsToString(Client.GetReceivedEvents()));
      try {
        Handle(&Poll, &Client);
      } catch (const std::system_error &E) {
        if (IsTerminated())
          break;
        Log::DefaultLogger.LogFatal("[Client #", Client.GetFD(),
                                    "]: ", E.what());
        Finish();
      }
      ThisThread::InterruptionPoint();
    }
  }
  Log::DefaultLogger.LogInfo("Shutting down...");
}

void ServerHandler::Start() { ServerThread.StartThread(); }

void ServerHandler::Handle(Poller *P, PollClient *Client) {
  auto Events = Client->GetReceivedEvents();

  if (Events & (POLLHUP | POLLNVAL | POLLERR)) {
    Finish();
    return;
  }

  if (!(Events & POLLIN))
    return;

  assert(Client->GetFD() == Sock->GetFD());

  Socket *ClientSocket = Sock->Accept();
  auto *Handler = new ClientHandler(Srv, ClientSocket);
  Srv->RegisterHandler(Handler);
  Handler->Start();
}

ServerHandler::~ServerHandler() {
  Terminate();
  ServerThread.Interrupt();
  ServerThread.Join();
  if (Sock)
    delete Sock;
}
} // namespace proxy
