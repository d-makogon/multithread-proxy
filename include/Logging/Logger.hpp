#pragma once
#include <Parallel/Thread.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <time.h>

namespace proxy {
// Returns current time in HH:mm:ss format
class Thread;

static std::string CurrentTime() {
  using namespace std::chrono;
  using clock = system_clock;

  const auto CurrentTimePoint{clock::now()};
  const auto CurrentTime{clock::to_time_t(CurrentTimePoint)};
  const auto CurrentLocaltime{*std::localtime(&CurrentTime)};
  const auto CurrentTimeSinceEpoch{CurrentTimePoint.time_since_epoch()};
  const auto CurrentMilliseconds{
      duration_cast<milliseconds>(CurrentTimeSinceEpoch).count() % 1000};

  std::ostringstream stream;
  stream << std::put_time(&CurrentLocaltime, "%T") << "." << std::setw(3)
         << std::setfill('0') << CurrentMilliseconds;
  return stream.str();
}

namespace Log {
enum class Level { Debug, Info, Error, Fatal, None };
} // namespace Log

namespace convert {
static std::string to_string(std::string s) { return s; }

static std::string to_string(Thread::Id TID) {
  std::ostringstream OS;
  OS << TID;
  return OS.str();
}

template <class T> static std::string stringify(T &&t) {
  using convert::to_string;
  using std::to_string;
  return to_string(std::forward<T>(t));
}
} // namespace convert

template <typename OStreamT> class Logger {
public:
  explicit Logger(OStreamT *Stream)
      : Stream(Stream), ThreadInfoEnabled(false),
        MinimumLevel(Log::Level::Info) {}

  Logger(OStreamT *Stream, bool ThreadInfoEnabled)
      : Stream(Stream), ThreadInfoEnabled(ThreadInfoEnabled),
        MinimumLevel(Log::Level::Info) {}

  Logger(OStreamT *Stream, bool ThreadInfoEnabled, Log::Level MinimumLevel)
      : Stream(Stream), ThreadInfoEnabled(ThreadInfoEnabled),
        MinimumLevel(MinimumLevel) {}

  void SetStream(OStreamT *Stream) { this->Stream = Stream; }

  void SetMinimumLevel(Log::Level Lvl) { MinimumLevel = Lvl; }

  void SetThreadInfoEnabled(bool ThreadInfoEnabled) {
    this->ThreadInfoEnabled = ThreadInfoEnabled;
  }

  void Log(Log::Level Lvl, std::string Message) {
    if (Lvl < MinimumLevel || Lvl == Log::Level::None)
      return;
    std::ostringstream ss;
    ss << CurrentTime() << std::left << std::setw(10)
       << (std::string(" [") + LevelToString(Lvl) + "]");
    if (ThreadInfoEnabled)
      ss << "[Thread #" << ThisThread::GetId() << "] ";

    ss << Message << "\n";
    *Stream << ss.str();
  }

  template <typename... Args> void LogDebug(Args &&... args) {
    Log(Log::Level::Debug,
        (convert::stringify<Args>(std::forward<Args>(args)) + ...));
  }

  template <typename... Args> void LogInfo(Args &&... args) {
    Log(Log::Level::Info,
        (convert::stringify<Args>(std::forward<Args>(args)) + ...));
  }

  template <typename... Args> void LogError(Args &&... args) {
    Log(Log::Level::Error,
        (convert::stringify<Args>(std::forward<Args>(args)) + ...));
  }

  template <typename... Args> void LogFatal(Args &&... args) {
    Log(Log::Level::Fatal,
        (convert::stringify<Args>(std::forward<Args>(args)) + ...));
  }

  ~Logger() { Stream->flush(); }

private:
  bool ThreadInfoEnabled;
  OStreamT *Stream;
  Log::Level MinimumLevel;

  static const char *LevelToString(const Log::Level &Lvl) {
    switch (Lvl) {
    case Log::Level::Debug:
      return "DEBUG";
    case Log::Level::Info:
      return "INFO";
    case Log::Level::Error:
      return "ERROR";
    case Log::Level::Fatal:
      return "FATAL";
    }
    return "UNKNOWN";
  }
};

namespace Log {
using DefaultLoggerT = Logger<std::ostream>;
extern DefaultLoggerT DefaultLogger;
} // namespace Log
} // namespace proxy
