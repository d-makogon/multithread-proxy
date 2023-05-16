#pragma once
#include <Common/Utils.hpp>
#include <Net/ServerSocket.hpp>
#include <Net/SocketBase.hpp>
#include <Parallel/Mutex.hpp>
#include <chrono>
#include <memory>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace proxy {
class Socket : public SocketBase {
private:
  struct sockaddr_storage Addr;
  struct addrinfo AddrInfo;

  Socket() : Socket(-1) {}

  explicit Socket(int Fd) : SocketBase(Fd) {
    AddrInfo.ai_addr = reinterpret_cast<struct sockaddr *>(&Addr);
    AddrInfo.ai_addrlen = sizeof(Addr);
  }

  struct addrinfo *GetAddrInfo();

public:
  static Socket *AcceptFrom(SocketBase *SB);

  static Socket *ConnectTo(const std::string &Host, uint16_t Port);

  Socket(const Socket &) = delete;
  Socket(Socket &&) = default;
  Socket &operator=(const Socket &) = delete;
  Socket &operator=(Socket &&) = default;

  ssize_t Write(const std::vector<char> &Bytes);
  ssize_t Write(const char *Bytes, std::size_t Size);
  ssize_t ReadAppend(std::vector<char> &Bytes);
  ssize_t Read(std::vector<char> &Bytes);
  ssize_t Read(char *Bytes, std::size_t Size);

  bool CanWriteWithoutBlocking();
  ~Socket() override;
};
} // namespace proxy
