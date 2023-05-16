#pragma once
#include <cerrno>
#include <string>
#include <system_error>

namespace proxy {
struct Exception {
  static void ThrowSystemError(const std::string &Message = "") {
    Exception::ThrowSystemError(errno, Message);
  }
  
  static void ThrowSystemError(int Error, const std::string &Message = "") {
    if (Message.empty())
      throw std::system_error(Error, std::generic_category());
    throw std::system_error(Error, std::generic_category(), Message);
  }
};
} // namespace proxy
