#include <Common/Globals.hpp>
#include <Common/Utils.hpp>
#include <Logging/Logger.hpp>
#include <Net/Socket.hpp>
#include <Parallel/LockGuard.hpp>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <poll.h>

namespace proxy {
static std::string AddrToStr(struct addrinfo *AddrInfo) {
  if (!AddrInfo)
    return "";
  char Host[50];
  char Port[6];
  int Status = getnameinfo(AddrInfo->ai_addr, AddrInfo->ai_addrlen, Host,
                           sizeof(Host), Port, sizeof(Port),
                           NI_NUMERICSERV | NI_NUMERICHOST | NI_NOFQDN);
  ThisThread::InterruptionPoint();
  if (Status != 0)
    return "";

  return std::string(Host) + ":" + std::string(Port);
}

struct addrinfo *Socket::GetAddrInfo() {
  return &AddrInfo;
}

Socket::~Socket() {
  if (0 != close(Fd))
    Log::DefaultLogger.LogError("close(fd): ", strerror(errno));
}

ssize_t Socket::Write(const char *Bytes, std::size_t Size) {
  ssize_t SentBytes;
  SentBytes = send(Fd, Bytes, Size, MSG_NOSIGNAL);
  ThisThread::InterruptionPoint();
  UpdateLastIOTimePoint();
  if (SentBytes < 0)
    Exception::ThrowSystemError("send()");
  return SentBytes;
}

ssize_t Socket::Read(char *Bytes, std::size_t Size) {
  ssize_t ReceivedBytes;
  ReceivedBytes = recv(Fd, Bytes, Size, MSG_NOSIGNAL);
  ThisThread::InterruptionPoint();
  UpdateLastIOTimePoint();
  if (ReceivedBytes < 0)
    Exception::ThrowSystemError("recv()");
  return ReceivedBytes;
}

ssize_t Socket::Write(const std::vector<char> &Bytes) {
  return Write(Bytes.data(), Bytes.size());
}

ssize_t Socket::ReadAppend(std::vector<char> &Bytes) {
  char Buf[Globals::DefaultReadBufferSize];
  ssize_t ReceivedBytes = Read(Buf, sizeof(Buf));
  Bytes.insert(Bytes.end(), Buf, Buf + ReceivedBytes);
  return ReceivedBytes;
}

ssize_t Socket::Read(std::vector<char> &Bytes) {
  char Buf[Globals::DefaultReadBufferSize];
  Bytes.clear();
  ssize_t ReceivedBytes = Read(Buf, sizeof(Buf));
  Bytes.insert(Bytes.begin(), Buf, Buf + ReceivedBytes);
  return ReceivedBytes;
}

bool Socket::CanWriteWithoutBlocking() {
  struct pollfd pfd;
  pfd.fd = Fd;
  pfd.events = POLLOUT;
  pfd.revents = 0;
  int Status = poll(&pfd, 1, 0);
  ThisThread::InterruptionPoint();
  if (Status == -1)
    Exception::ThrowSystemError("recv()");
  return Status == 1;
}

Socket *Socket::AcceptFrom(SocketBase *SB) {
  Socket *Sock = new Socket();

  struct addrinfo *SockAddrInfo = Sock->GetAddrInfo();
  int SocketFd =
      accept(SB->GetFD(), SockAddrInfo->ai_addr, &SockAddrInfo->ai_addrlen);
  try {
    ThisThread::InterruptionPoint();
    if (SocketFd == -1)
      Exception::ThrowSystemError("accept()");
  } catch (...) {
    delete Sock;
    throw;
  }
  SB->UpdateLastIOTimePoint();
  Sock->SetFd(SocketFd);
  Sock->UpdateLastIOTimePoint();
  return Sock;
}

Socket *Socket::ConnectTo(const std::string &Host, uint16_t Port) {
  int FD;
  struct addrinfo *AddrInfos;
  struct addrinfo Hints;

  memset(&Hints, 0, sizeof(Hints));
  Hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
  Hints.ai_socktype = SOCK_STREAM; // TCP

  int Status = getaddrinfo(Host.c_str(), std::to_string(Port).c_str(), &Hints,
                           &AddrInfos);
  ThisThread::InterruptionPoint();
  if (Status)
    Exception::ThrowSystemError(Status, std::string("getaddrinfo()") +
                                            gai_strerror(Status));

  int yes = 1;
  // Choose the first appropriate addr filled by gai
  struct addrinfo *Info = nullptr;
  for (Info = AddrInfos; Info != nullptr; Info = Info->ai_next) {
    if ((FD = socket(Info->ai_family, Info->ai_socktype, Info->ai_protocol)) ==
        -1) {
      ThisThread::InterruptionPoint();
      Log::DefaultLogger.LogError("socket()", strerror(errno));
      continue;
    }
    break;
  }

  if (!Info) {
    freeaddrinfo(AddrInfos);
    close(FD);
    Log::DefaultLogger.LogError("Failed to resolve ", Host, ":", Port);
    Exception::ThrowSystemError("getaddrinfo()");
  }

  Socket *Sock = new Socket(FD);
  memcpy(Sock->GetAddrInfo(), Info, sizeof(*Info));

  struct addrinfo *AddrInfo = Sock->GetAddrInfo();
  Status = connect(FD, AddrInfo->ai_addr, AddrInfo->ai_addrlen);
  ThisThread::InterruptionPoint();
  if (Status < 0 && errno != EINPROGRESS) {
    close(FD);
    freeaddrinfo(AddrInfos);
    Log::DefaultLogger.LogError("Failed to connect to ", Host, ":", Port);
    Exception::ThrowSystemError("connect()");
  }
  Sock->UpdateLastIOTimePoint();
  freeaddrinfo(AddrInfos);
  Log::DefaultLogger.LogInfo("Resolved ", Host, ":", Port, " to ",
                             AddrToStr(Sock->GetAddrInfo()));
  return Sock;
}
} // namespace proxy
