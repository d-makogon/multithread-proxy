#include <Common/ProxyException.hpp>
#include <Functional/Function.hpp>
#include <Logging/Logger.hpp>
#include <Parallel/LockGuard.hpp>
#include <Parallel/Once.hpp>
#include <Parallel/PthreadHelpers.hpp>
#include <Parallel/Thread.hpp>
#include <Parallel/ThreadInterruptedException.hpp>
#include <cstring>
#include <pthread.h>
#include <sstream>

namespace proxy {
namespace {
OnceFlag CurrentThreadTLSInitFlag = OnceFlag();
pthread_key_t CurrentThreadTLSKey;

extern "C" {
static void TLSDestructor(void *Data) {
  ThreadDataPtr TData = static_cast<ThreadDataBase *>(Data)->Self;
  if (TData)
    TData->Self.reset();
}
}

void CreateCurrentThreadTLSKey() {
  VERIFY_PTHREAD_CALL(pthread_key_create(&CurrentThreadTLSKey, &TLSDestructor));
}

ThreadDataBase *GetCurrentThreadData() {
  CallOnce(CurrentThreadTLSInitFlag, Function(&CreateCurrentThreadTLSKey));
  return reinterpret_cast<ThreadDataBase *>(
      pthread_getspecific(CurrentThreadTLSKey));
}

void SetCurrentThreadData(ThreadDataBase *NewData) {
  CallOnce(CurrentThreadTLSInitFlag, Function(&CreateCurrentThreadTLSKey));
  VERIFY_PTHREAD_CALL(pthread_setspecific(CurrentThreadTLSKey, NewData));
}

extern "C" {
static void *ThreadProxy(void *Arg) {
  ThreadDataPtr ThreadData = (static_cast<ThreadDataBase *>(Arg))->Self;
  ThreadData->Self.reset();
  SetCurrentThreadData(ThreadData.get());
  try {
    ThreadData->Run();
  } catch (const ThreadInterruptedException &E) {
    Log::DefaultLogger.LogInfo("Thread interrupted");
  }
  LockGuard<MutexLocker> Guard(ThreadData->DataMutex.get(),
                               /*CheckInterrupt=*/false);
  ThisThread::DisableInterruption();
  TLSDestructor(ThreadData.get());
  SetCurrentThreadData(nullptr);
  ThreadData->Done = true;
  ThreadData->DoneCondition->NotifyAll();
  return nullptr;
}
}
} // namespace

Thread::Thread(Thread &&T) {
  OwnThreadData = T.OwnThreadData;
  T.OwnThreadData.reset();
}

bool Thread::StartThreadNoExcept() {
  int Status;
  bool Success = StartThreadNoExcept(Status);
  ThisThread::InterruptionPoint();
  return Success;
}

bool Thread::StartThreadNoExcept(int &Status) {
  OwnThreadData->Self = OwnThreadData;
  Status = pthread_create(&OwnThreadData->ThreadHandle, 0, &ThreadProxy,
                          OwnThreadData.get());
  if (Status != 0) {
    OwnThreadData->Self.reset();
    return false;
  }
  return true;
}

void Thread::StartThread() {
  SetInterruptSignalHandler();
  int Status;
  if (!StartThreadNoExcept(Status))
    Exception::ThrowSystemError(Status, "pthread_create");
}

void Thread::Detach() {
  ThreadDataPtr LocalThreadData;
  OwnThreadData.swap(LocalThreadData);

  if (LocalThreadData) {
    LockGuard<MutexLocker> Guard(LocalThreadData->DataMutex.get());
    if (!LocalThreadData->JoinStarted) {
      int Status = pthread_detach(LocalThreadData->ThreadHandle);
      ThisThread::InterruptionPoint();
      if (Status == -1)
        Exception::ThrowSystemError(Status, "pthread_detach");

      LocalThreadData->JoinStarted = true;
      LocalThreadData->Joined = true;
    }
  }
}

bool Thread::Joinable() const { return OwnThreadData ? true : false; }

void Thread::Join() {
  auto Id = GetId();
  if (ThisThread::GetId() == Id)
    throw std::logic_error("Attempted to join self!");

  Log::DefaultLogger.LogInfo("Joining thread #", Id);
  int Status;
  if (!JoinNoExcept(Status))
    Exception::ThrowSystemError(Status, "pthread_join");
  Log::DefaultLogger.LogInfo("Joined thread #", Id);
}

bool Thread::JoinNoExcept() {
  int Status;
  bool Success = JoinNoExcept(Status);
  ThisThread::InterruptionPoint();
  return Success;
}

bool Thread::JoinNoExcept(int &Status) {
  if (!OwnThreadData)
    return false;
  bool NeedJoin = false;
  {
    LockGuard<MutexLocker> Guard(OwnThreadData->DataMutex.get());

    while (!OwnThreadData->Done)
      OwnThreadData->DoneCondition->Wait(*OwnThreadData->DataMutex);

    NeedJoin = !OwnThreadData->JoinStarted;

    if (NeedJoin)
      OwnThreadData->JoinStarted = true;
    else
      while (!OwnThreadData->Joined)
        OwnThreadData->DoneCondition->Wait(*OwnThreadData->DataMutex);
  }

  if (NeedJoin) {
    int Status = pthread_join(OwnThreadData->ThreadHandle, nullptr);
    if (Status != 0)
      return false;
    LockGuard<MutexLocker> Guard(OwnThreadData->DataMutex.get());
    OwnThreadData->Joined = true;
    OwnThreadData->DoneCondition->NotifyAll();
  }
  OwnThreadData.reset();
  return true;
}

void Thread::Interrupt() {
  auto Id = GetId();
  Log::DefaultLogger.LogInfo("Interrupting thread #", Id);
  pthread_kill(Id, THREAD_INTERRUPT_SIG);
}

extern "C" {
void ThreadSigUsr1Handler(int Sig, siginfo_t *SigInfo, void *Arg) {
  if (Sig != THREAD_INTERRUPT_SIG)
    return;
  auto *ThreadData = GetCurrentThreadData();
  if (!ThreadData)
    return;
  LockGuard<MutexLocker> G(ThreadData->DataMutex.get(),
                           /*CheckInterrupt=*/false);
  ThreadData->InterruptRequested = true;
}
}

void Thread::SetInterruptSignalHandler() {
  struct sigaction OnSignalAction;
  memset(&OnSignalAction, 0, sizeof(OnSignalAction));
  OnSignalAction.sa_sigaction = &ThreadSigUsr1Handler;

  if (0 != sigaction(THREAD_INTERRUPT_SIG, &OnSignalAction, NULL))
    Exception::ThrowSystemError("sigaction()");
}

bool Thread::InterruptRequested() {
  if (!OwnThreadData)
    return false;
  LockGuard<MutexLocker> Guard(OwnThreadData->DataMutex.get(),
                               /*CheckInterrupt=*/false);
  bool InterruptRequested = OwnThreadData->InterruptRequested;
  return InterruptRequested;
}

pthread_t Thread::GetHandle() {
  if (!OwnThreadData)
    return pthread_t();
  LockGuard<MutexLocker> Guard(OwnThreadData->DataMutex.get());
  pthread_t Handle = OwnThreadData->ThreadHandle;
  return Handle;
}

Thread::Id Thread::GetId() const {
  return Thread::Id(const_cast<Thread *>(this)->GetHandle());
}

namespace ThisThread {
void InterruptionPoint() {
  ThreadDataBase *TData = GetCurrentThreadData();
  if (!TData || !TData->InterruptEnabled)
    return;
  LockGuard<MutexLocker> Guard(TData->DataMutex.get(),
                               /*CheckInterrupt=*/false);
  if (TData->InterruptRequested) {
    TData->InterruptRequested = false;
    throw ThreadInterruptedException();
  }
}

bool InterruptionEnabled() {
  ThreadDataBase *TData = GetCurrentThreadData();
  return TData && TData->InterruptEnabled;
}

bool InterruptionRequested() {
  ThreadDataBase *TData = GetCurrentThreadData();
  if (!TData)
    return false;
  LockGuard<MutexLocker> Guard(TData->DataMutex.get(),
                               /*CheckInterrupt=*/false);
  bool Requested = TData->InterruptRequested;
  return Requested;
}

void EnableInterruption() {
  ThreadDataBase *TData = GetCurrentThreadData();
  if (!TData)
    return;
  TData->InterruptEnabled = true;
}

void BlockInterruptionSignals() {
  ThreadDataBase *TData = GetCurrentThreadData();
  if (!TData)
    return;
  TData->BlockInterruptionSignals();
}

void DisableInterruption() {
  ThreadDataBase *TData = GetCurrentThreadData();
  if (!TData)
    return;
  TData->InterruptEnabled = false;
}
} // namespace ThisThread
} // namespace proxy
