#include <Common/ProxyException.hpp>
#include <Common/Utils.hpp>
#include <Logging/Logger.hpp>
#include <Net/Poller.hpp>
#include <Parallel/LockGuard.hpp>
#include <bitset>
#include <cerrno>
#include <cstring>
#include <vector>

namespace proxy {
void Poller::Add(int Fd, short Events, PollHandlerBase *Handler) {
  Log::DefaultLogger.LogDebug("Added FD=", Fd, " (",
                              Utils::EventsToString(Events), ") to poll pool");
  PollUpdates.emplace_back(std::make_pair(PollUpdateType::AddClient,
                                          PollClient(Fd, Events, 0, Handler)));
}

void Poller::Remove(int Fd, short Events) {
  Log::DefaultLogger.LogDebug("Removed FD=", Fd, " (",
                              Utils::EventsToString(Events),
                              ") from poll pool");
  PollUpdates.emplace_back(
      std::make_pair(PollUpdateType::RemoveClient, PollClient(Fd, Events)));
}

void Poller::AddClient(PollClient &Client) {
  int FD = Client.GetFD();
  short Events = Client.GetPollEvents();
  auto *Handler = Client.GetHandler();
  auto It = std::find_if(PollFDs.begin(), PollFDs.end(),
                         [&](struct pollfd &PFD) { return PFD.fd == FD; });
  if (It != PollFDs.end()) {
    It->events |= Events;
    auto &Cl = Clients[FD];
    Cl.SetHandler(Handler);
    Cl.SetPollEvents(Cl.GetPollEvents() | Events);
    return;
  }

  PollFDs.push_back({.fd = FD, .events = Events, .revents = 0});
  Clients[FD] = Client;
}

void Poller::RemoveClient(const PollClient &Client) {
  int FD = Client.GetFD();
  short Events = Client.GetPollEvents();
  for (auto It = PollFDs.begin(); It != PollFDs.end();) {
    if (It->fd == FD) {
      It->events &= ~Events;
      // If no events will be polled for this Fd anymore, remove it completely
      if (It->events == 0) {
        It = PollFDs.erase(It);
        Clients.erase(FD);
        continue;
      }
    }
    ++It;
  }
}

void Poller::Flush() {
  for (auto &[Type, Client] : PollUpdates) {
    if (Type == PollUpdateType::AddClient)
      AddClient(Client);
    else
      RemoveClient(Client);
  }
  PollUpdates.clear();
}

void Poller::ResetReceivedEvents() {
  for (auto &PFD : PollFDs)
    PFD.revents = 0;
  for (auto &[FD, Client] : Clients)
    Client.SetReceivedEvents(0);
}

Poller::PolledClientsIterator Poller::begin() {
  auto FirstPolled = std::find_if(PollFDs.begin(), PollFDs.end(),
                                  [](auto &PFD) { return PFD.revents != 0; });
  return PolledClientsIterator(Clients, FirstPolled, PollFDs.end());
}

Poller::PolledClientsIterator Poller::end() {
  return PolledClientsIterator(Clients, PollFDs.end());
}

std::size_t Poller::GetPolledClientsNum() const { return PollFDs.size(); }

int Poller::Poll(int TimeoutMSec) {
  Flush();
  ResetReceivedEvents();
  if (PollFDs.size() == 0)
    return 0;
  int Status = poll(PollFDs.data(), PollFDs.size(), TimeoutMSec);
  ThisThread::InterruptionPoint();
  if (Status == -1)
    Exception::ThrowSystemError("poll()");

  Log::DefaultLogger.LogDebug("Polled ", Status, " clients");

  std::size_t PFDNum = 0;
  for (auto &PFD : PollFDs) {
    if (PFDNum++ == Status)
      break;
    auto &Cl = Clients[PFD.fd];
    Cl.SetReceivedEvents(PFD.revents);
    Cl.SetPollEvents(PFD.events);
  }
  return Status;
}
} // namespace proxy
