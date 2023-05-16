#include <Common/Globals.hpp>
#include <Common/Utils.hpp>
#include <Functional/Function.hpp>
#include <Logging/Logger.hpp>
#include <Net/Server.hpp>
#include <Parallel/Thread.hpp>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <httpparser/httprequestparser.h>
#include <httpparser/request.h>
#include <iostream>
#include <thread>
#include <unordered_map>

using namespace httpparser;
using namespace proxy;

int main(int argc, char const *argv[]) {
  if (argc != 2) {
    std::cerr << "Expected a port argument" << std::endl;
    return 1;
  }

  uint16_t Port;
  if (!Utils::StrToInt<uint16_t>(Port, std::string(argv[1]))) {
    std::cerr << "Couldn't convert " << argv[1] << " to a number" << std::endl;
    return 1;
  }

  Log::DefaultLogger.SetMinimumLevel(Log::Level::Fatal);
  Log::DefaultLogger.SetThreadInfoEnabled(true);

  auto Srv = std::make_unique<Server>(Port);
  try {
    Thread SrvThread(Function(&Server::Start, Srv.get()));
    Utils::BlockInterruptionSignals();
    SrvThread.StartThread();
    SrvThread.Join();
  } catch (std::system_error &E) {
    Log::DefaultLogger.LogFatal(E.what());
  }
  Srv.reset(nullptr);
  pthread_exit(NULL);
  return 0;
}
