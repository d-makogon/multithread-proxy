#pragma once
#include <Net/Socket.hpp>
#include <Net/SocketBase.hpp>
#include <cstdint>

namespace proxy {
class Socket;
class ServerSocket : public SocketBase {
private:
  uint16_t Port;

public:
  explicit ServerSocket(uint16_t Port);
  void Listen();
  Socket *Accept();
  int GetPort() const;
  ~ServerSocket() override;
};
} // namespace proxy
