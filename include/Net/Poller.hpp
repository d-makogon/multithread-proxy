#pragma once

#include <Net/PollHandlerBase.hpp>
#include <Net/SocketBase.hpp>
#include <Parallel/Mutex.hpp>
#include <cstddef>
#include <deque>
#include <functional>
#include <iterator>
#include <memory>
#include <poll.h>
#include <set>
#include <unordered_map>
#include <vector>

namespace proxy {

class PollHandlerBase;

class PollClient {
  int FD;
  short PollEvents;
  short ReceivedEvents;
  PollHandlerBase *Handler = nullptr;

public:
  PollClient(int FD = -1, short PollEvents = 0, short ReceivedEvents = 0,
             PollHandlerBase *Handler = nullptr)
      : FD(FD), PollEvents(PollEvents), ReceivedEvents(ReceivedEvents),
        Handler(Handler) {}

  int GetFD() const { return FD; }
  short GetPollEvents() const { return PollEvents; }
  void SetPollEvents(short PollEvents) { this->PollEvents = PollEvents; }
  void SetReceivedEvents(short Events) { ReceivedEvents = Events; }
  short GetReceivedEvents() const { return ReceivedEvents; }
  void SetHandler(PollHandlerBase *HB) { Handler = HB; }
  PollHandlerBase *GetHandler() { return Handler; }
  const PollHandlerBase *GetHandler() const { return Handler; }
};

class Poller {
private:
  std::vector<struct pollfd> PollFDs;
  std::set<int> ResetReventsFDs;

  using ClientsMapT = std::unordered_map<int, PollClient>;
  ClientsMapT Clients;

  enum class PollUpdateType { AddClient, RemoveClient };
  std::deque<std::pair<PollUpdateType, PollClient>> PollUpdates;

  void AddClient(PollClient &Client);
  void RemoveClient(const PollClient &Client);
  void ResetReceivedEvents();

public:
  using PollFDsIterator = std::vector<struct pollfd>::iterator;

  struct PolledClientsIterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = PollClient;
    using pointer = value_type *;
    using reference = value_type &;

    PolledClientsIterator(ClientsMapT &ClientsMap, PollFDsIterator Iter,
                          PollFDsIterator End = PollFDsIterator())
        : ClientsMap(ClientsMap), Iter(Iter), End(End) {}

    reference operator*() const {
      reference Client = ClientsMap[Iter->fd];
      Client.SetReceivedEvents(Iter->revents);
      return Client;
    }

    pointer operator->() {
      pointer Client = &ClientsMap[Iter->fd];
      Client->SetReceivedEvents(Iter->revents);
      return Client;
    }

    PolledClientsIterator &operator++() {
      while (++Iter < End && Iter->revents == 0)
        ;
      return *this;
    }

    PolledClientsIterator operator++(int) {
      PolledClientsIterator Tmp = *this;
      ++(*this);
      return Tmp;
    }

    friend bool operator==(const PolledClientsIterator &A,
                           const PolledClientsIterator &B) {
      return A.Iter == B.Iter;
    };

    friend bool operator!=(const PolledClientsIterator &A,
                           const PolledClientsIterator &B) {
      return A.Iter != B.Iter;
    };

  private:
    ClientsMapT &ClientsMap;
    PollFDsIterator Iter;
    PollFDsIterator End;
  };

  explicit Poller(std::size_t InitialSize = 0) : Clients(InitialSize) {}

  // Queues the pollfd client addition until flushed.
  void Add(int FD, short Events, PollHandlerBase *Handler);

  // Queues the pollfd client removal until flushed.
  void Remove(int FD, short Events = POLLIN | POLLPRI | POLLOUT | POLLERR |
                                     POLLHUP | POLLNVAL);

  // Flushes all the queued pollfd addition/removals.
  void Flush();

  PolledClientsIterator begin();
  PolledClientsIterator end();

  std::size_t GetPolledClientsNum() const;
  int Poll(int TimeoutMSec);

  ~Poller() = default;
};
} // namespace proxy
