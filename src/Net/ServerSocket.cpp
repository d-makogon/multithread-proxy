#include <Common/ProxyException.hpp>
#include <Logging/Logger.hpp>
#include <Net/ServerSocket.hpp>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

#include <pthread.h>

namespace proxy {
ServerSocket::ServerSocket(uint16_t Port) {
  this->Port = Port;
  Fd = socket(AF_INET6, SOCK_STREAM, 0);
  if (Fd == -1)
    Exception::ThrowSystemError("socket()");

  // Accept IPv4 as well as IPv6
  int IPV6Only = 0;
  if (setsockopt(Fd, IPPROTO_IPV6, IPV6_V6ONLY, &IPV6Only, sizeof(IPV6Only)) ==
      -1) {
    close(Fd);
    Exception::ThrowSystemError("setsockopt()");
  }

  int ReuseAddr = 1;
  if (setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, &ReuseAddr, sizeof(ReuseAddr)) ==
      -1) {
    close(Fd);
    Exception::ThrowSystemError("setsockopt()");
  }

  struct sockaddr_in6 Addr;
  memset(&Addr, 0, sizeof(Addr));
  Addr.sin6_addr = in6addr_any;
  Addr.sin6_family = AF_INET6;
  Addr.sin6_port = htons(Port);

  if (bind(Fd, reinterpret_cast<const sockaddr *>(&Addr), sizeof(Addr)) == -1) {
    close(Fd);
    Exception::ThrowSystemError("bind()");
  }
}

void ServerSocket::Listen() {
  if (listen(Fd, SOMAXCONN) == -1) {
    close(Fd);
    Exception::ThrowSystemError("listen()");
  }
}

Socket *ServerSocket::Accept() {
  auto *S = Socket::AcceptFrom(this);
  Log::DefaultLogger.LogInfo("Accepted new connection at port ", Port,
                             " at FD ", S->GetFD());
  return S;
}

int ServerSocket::GetPort() const { return Port; }

ServerSocket::~ServerSocket() {
  Log::DefaultLogger.LogInfo("Closing FD=", Fd, " (server socket)");
  if (0 != close(Fd))
    Log::DefaultLogger.LogError("close(fd): ", strerror(errno));
}
} // namespace proxy
