#include <Common/ProxyException.hpp>
#include <Common/Utils.hpp>
#include <Logging/Logger.hpp>
#include <Net/ClientHandler.hpp>
#include <Net/RemoteHandler.hpp>
#include <Parallel/LockGuard.hpp>
#include <Parallel/ThreadInterruptedException.hpp>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <httpparser/httprequestparser.h>
#include <httpparser/httpresponseparser.h>
#include <httpparser/request.h>
#include <httpparser/urlparser.h>

namespace proxy {
SocketBase *ClientHandler::GetSocket() { return ClientSock; }

std::optional<SocketBase::TimePointT> ClientHandler::GetLastIOTimePoint() {
  if (ClientSock)
    return ClientSock->GetLastIOTimePoint();
  return {};
}

void ClientHandler::Finish() {
  Terminate();
  Srv->MarkDeadHandler(this);
}

void ClientHandler::Terminate() {
  {
    LockGuard<MutexLocker> G(&IsTerminatedMutex);
    if (_IsTerminated)
      return;
    _IsTerminated = true;
  }
  if (!CacheAddress.empty())
    Srv->GetCache()->RemoveListener(CacheAddress, this);
  if (EndToEndHandler)
    EndToEndHandler->Terminate();
}

void ClientHandler::HandleWriteEvents() {
  if (IsEndToEnd) {
    HandleEndToEndWrite();
    return;
  }
  std::size_t BlocksNum;
  {
    LockGuard<MutexLocker> G(&CacheEventMutex);
    BlocksNum = CacheBlocksNum;
  }
  if (BlocksNum > 0)
    SendRecordFromCache();
}

void ClientHandler::SendRecordFromCache() {
  LockGuard<MutexLocker> G(&CacheEventMutex);
  std::size_t CurCacheBlocksNum = CacheBlocksNum;
  auto *CurBlock = ResponseCacheRecord->GetBlock(CurBlockIdx);
  if (!CurBlock) {
    Poll.Remove(ClientSock->GetFD(), POLLOUT);
    return;
  }
  G.Unlock();

  const auto &CurBlockBytes = CurBlock->GetBytes();
  if (CurBlockBytes.size() == 0 && !CurBlock->IsFinal()) {
    Poll.Remove(ClientSock->GetFD(), POLLOUT);
    return;
  }

  auto BytesStartIt = CurBlockBytes.begin() + CurBlockPos;
  std::size_t BytesNum = CurBlockBytes.size() - CurBlockPos;

  auto *NextBlock = ResponseCacheRecord->GetBlock(CurBlockIdx + 1);
  std::vector<char> Concat;
  if (NextBlock && BytesNum < Globals::BufferConcatSizeThreshold) {
    const auto &NextBlockBytes = NextBlock->GetBytes();
    Utils::ConcatRanges(BytesStartIt, CurBlockBytes.end(),
                        NextBlockBytes.begin(), NextBlockBytes.end(),
                        std::back_inserter(Concat));
    BytesStartIt = Concat.begin();
    BytesNum = Concat.size();
  }

  ssize_t WrittenBytesNum = ClientSock->Write(&*BytesStartIt, BytesNum);

  Log::DefaultLogger.LogDebug("[Client #", ClientSock->GetFD(), "] Sent ",
                              WrittenBytesNum, " bytes from cache");

  CurBlockPos += WrittenBytesNum;
  bool FinishedCurBlock = CurBlockPos >= CurBlockBytes.size();
  if (FinishedCurBlock) {
    CurBlockPos -= CurBlockBytes.size();
    CurBlockIdx++;
    if (CurBlockIdx == CurCacheBlocksNum)
      Poll.Remove(ClientSock->GetFD(), POLLOUT);
  }

  if (CurBlock->IsFinal() && FinishedCurBlock) {
    if (ResponseCacheRecord->IsComplete()) {
      Poll.Remove(SockFD, POLLOUT);
      Log::DefaultLogger.LogInfo("[Client #", SockFD,
                                 "] Finished reading from cache");
      Finish();
      return;
    }

    // Cache record is finished, but response is not complete. Need
    // to establish a new connection to remote to get the remaining response.
    IsEndToEnd = true;
    auto RangeValue = std::string("bytes=") +
                      std::to_string(ResponseCacheRecord->GetTotalSize()) + "-";
    auto RangeHeader =
        httpparser::Request::HeaderItem{.name = "Range", .value = RangeValue};
    ClientRequest.headers.push_back(RangeHeader);
    RequestBytes.clear();
    auto RequestStr = Utils::RequestToString(ClientRequest);
    RequestBytes.insert(RequestBytes.begin(), RequestStr.begin(),
                        RequestStr.end());
    EndToEndHandler = new RemoteHandler(Srv, CacheAddress, RequestBytes,
                                        RemoteHandler::Mode::EndToEnd);
    EndToEndHandler->SetEndToEndBuffer(&ResponseBuffer);
    EndToEndHandler->ConnectTo(RemoteHostName, RemoteHostPort);
    EndToEndHandler->Start();
    return;
  }

  // Wait until new cache block arrives
  WaitForCacheRecordUpdate(CurBlockIdx);
}

void ClientHandler::HandleRemoteEndInput(RemoteHandler *RemHandler,
                                         std::vector<char> *Bytes) {
  Poll.Add(SockFD, POLLOUT, this);
  RemoteInput = Bytes;
}

void ClientHandler::HandleEndToEndWrite() {
  if (RemoteInput->size() == 0) {
    Log::DefaultLogger.LogInfo("[Client #", SockFD,
                               "] Terminating end-to-end connection");
    Finish();
    return;
  }
  if (!ReadHeader) {
    const char *CRLF = "\r\n\r\n";
    auto HeadersEndIt = std::search(RemoteInput->begin(), RemoteInput->end(),
                                    CRLF, CRLF + strlen(CRLF));
    if (HeadersEndIt == RemoteInput->end()) {
      Log::DefaultLogger.LogInfo(
          "[Client #", SockFD,
          "] No response header in end-to-end mode. Terminating");
      Finish();
      return;
    }

    httpparser::HttpResponseParser Parser;
    auto Res = Parser.parse(EndToEndResponse, RemoteInput->data(),
                            HeadersEndIt.base() + strlen(CRLF));
    if (Res != httpparser::HttpResponseParser::ParsingCompleted) {
      Log::DefaultLogger.LogError("[Client #", SockFD, "] HTTP Parsing failed");
      Finish();
      return;
    }
    // Have to receive 206 Partial Content
    if (EndToEndResponse.statusCode != 206) {
      Log::DefaultLogger.LogError(
          "[Client #", SockFD,
          "] Expected 206 Partial Content in end-to-end mode");
      Finish();
      return;
    }
    // Erase header part
    RemoteInput->erase(RemoteInput->begin(), HeadersEndIt + 4);
    ReadHeader = true;
  }
  ssize_t WrittenBytes = ClientSock->Write(*RemoteInput);
  Log::DefaultLogger.LogInfo("[Client #", SockFD, "] Sent ", WrittenBytes,
                             " bytes in end-to-end mode");
  EndToEndHandler->Start();
  Poll.Remove(SockFD, POLLOUT);
}

void ClientHandler::OnCacheRecordUpdate(CacheRecord *Record) {
  std::size_t UpdatesNum = 0;
  {
    LockGuard<MutexLocker> G(&CacheEventMutex);
    ResponseCacheRecord = Record;
    PrevCacheBlocksNum = CacheBlocksNum;
    CacheBlocksNum = Record->GetNumBlocks();
    UpdatesNum = CacheBlocksNum - PrevCacheBlocksNum;
  }
  for (std::size_t i = 0; i < UpdatesNum; i++)
    CacheEventSemaphore.Release();
}

void ClientHandler::WaitForCacheRecordUpdate(std::size_t LastBlocksNum) {
  Log::DefaultLogger.LogDebug("[Client #", SockFD,
                              "] Waiting for cache record...");

  // Wait until cache record arrives
  CacheEventSemaphore.Acquire();

  Log::DefaultLogger.LogDebug("[Client #", SockFD,
                              "] Finished waiting for cache record");

  Poll.Add(SockFD, POLLOUT, this);
}

static uint16_t ParsePort(std::string &HostHeader) {
  uint16_t DefaultPort = 80;
  uint16_t Port;
  std::size_t SemicolonPos = HostHeader.find_last_of(':');
  if (SemicolonPos != HostHeader.npos) {
    std::stringstream ss(HostHeader.substr(SemicolonPos + 1));
    ss >> Port;
    if (ss.fail() || ss.bad())
      return DefaultPort;
    HostHeader = HostHeader.substr(0, SemicolonPos);
    return Port;
  }
  return DefaultPort;
}

static bool TryGetHostHeader(const httpparser::Request &Req,
                             std::string &Host) {
  std::string HostHeader = "Host";
  auto IsHostHeader = [&](const auto &Header) {
    return std::equal(Header.name.begin(), Header.name.end(),
                      HostHeader.begin());
  };

  auto HostIt =
      std::find_if(Req.headers.begin(), Req.headers.end(), IsHostHeader);
  if (HostIt == Req.headers.end())
    return false;

  Host = HostIt->value;
  return true;
}

void ClientHandler::HandleClientInput() {
  ssize_t ReceivedBytes = ClientSock->ReadAppend(RequestBytes);

  Log::DefaultLogger.LogInfo("[Client #", SockFD, "] Received ", ReceivedBytes,
                             " bytes");

  if (ReceivedBytes == 0) {
    Finish();
    return;
  }

  if (RequestFinished)
    return;

  const char *CRLF = "\r\n\r\n";
  if (std::search(RequestBytes.begin(), RequestBytes.end(), CRLF,
                  CRLF + strlen(CRLF)) == RequestBytes.end())
    return;

  httpparser::HttpRequestParser HttpParser;
  httpparser::HttpRequestParser::ParseResult res =
      HttpParser.parse(ClientRequest, RequestBytes.data(),
                       RequestBytes.data() + RequestBytes.size());

  if (res != httpparser::HttpRequestParser::ParsingCompleted) {
    Log::DefaultLogger.LogError("[Client #", SockFD, "] HTTP Parsing failed");
    Finish();
    return;
  }

  Log::DefaultLogger.LogInfo(
      "[Client #", SockFD, "] ", ClientRequest.method, " ", ClientRequest.uri,
      " HTTP/", ClientRequest.versionMajor, ".", ClientRequest.versionMinor);

  if (ClientRequest.versionMajor >= 1 && ClientRequest.versionMinor >= 1) {
    Log::DefaultLogger.LogDebug(
        "[Client #", SockFD, "] ",
        "Unsupported HTTP version, falling back to HTTP/1.0");
    ClientRequest.versionMajor = 1;
    ClientRequest.versionMinor = 0;
    auto NewRequestStr = Utils::RequestToString(ClientRequest);
    RequestBytes.clear();
    RequestBytes.insert(RequestBytes.begin(), NewRequestStr.begin(),
                        NewRequestStr.end());
  }

  bool HasHostHeader = TryGetHostHeader(ClientRequest, RemoteHostName);

  CacheAddress = "";
  if (!HasHostHeader) {
    httpparser::UrlParser UrlParser(ClientRequest.uri);
    if (!UrlParser.isValid())
      throw std::runtime_error("Invalid URI: " + ClientRequest.uri);
    RemoteHostPort = UrlParser.httpPort();
    RemoteHostName = UrlParser.hostname();
    CacheAddress = ClientRequest.uri;
  } else {
    RemoteHostPort = ParsePort(RemoteHostName);
    CacheAddress += RemoteHostName + ClientRequest.uri + ":" +
                    std::to_string(RemoteHostPort);
  }

  CacheListenerInfo CLI{CacheAddress, RemoteHostName, RemoteHostPort,
                        RequestBytes, this};
  Srv->AddCacheListener(CLI);
  // Don't listen for client input anymore
  Poll.Remove(SockFD, POLLIN);
  RequestFinished = true;

  WaitForCacheRecordUpdate(0);
}

bool ClientHandler::IsTerminated() {
  LockGuard<MutexLocker> G(&IsTerminatedMutex);
  return _IsTerminated;
}

void ClientHandler::ClientRoutine() {
  ThisThread::BlockInterruptionSignals();
  Poll.Add(SockFD, POLLIN, this);
  while (!IsTerminated()) {
    Log::DefaultLogger.LogDebug("[Client #", SockFD, "] Waiting for events...");
    int NumPolled = Poll.Poll(Globals::ClientTimeoutMSec);
    if (NumPolled == 0) {
      if (Poll.GetPolledClientsNum() == 0) {
        WaitForCacheRecordUpdate(0);
        continue;
      }
      Finish();
      break;
    }

    for (auto &Client : Poll) {
      Log::DefaultLogger.LogDebug(
          "FD ", Client.GetFD(), " received ",
          Utils::EventsToString(Client.GetReceivedEvents()));

      try {
        Handle(&Poll, &Client);
      } catch (std::system_error &E) {
        Log::DefaultLogger.LogFatal("[Remote #", Client.GetFD(),
                                    "]: ", E.what());
        Finish();
        break;
      }
      ThisThread::InterruptionPoint();
    }
  }

  Log::DefaultLogger.LogDebug("[Client #", SockFD, "] Terminating");
}

void ClientHandler::Start() { ClientThread.StartThread(); }

void ClientHandler::Handle(Poller *P, PollClient *Client) {
  assert(Client->GetFD() == SockFD);
  auto Events = Client->GetReceivedEvents();

  if (Events & (POLLHUP | POLLNVAL | POLLERR)) {
    Log::DefaultLogger.LogInfo("[Client #", SockFD,
                               "] Client terminated connection");
    Finish();
    return;
  }

  if (Events & POLLIN)
    HandleClientInput();

  if (Events & POLLOUT)
    HandleWriteEvents();
}

ClientHandler::~ClientHandler() {
  Terminate();
  ClientThread.Interrupt();
  ClientThread.Join();
  if (ClientSock)
    delete ClientSock;
}
} // namespace proxy
