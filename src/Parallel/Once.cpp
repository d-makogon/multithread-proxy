#include <Common/ProxyException.hpp>
#include <Parallel/Once.hpp>
#include <Parallel/PthreadHelpers.hpp>
#include <cstdlib>

namespace proxy {
AtomicT OnceGlobalEpoch = ~0u;
pthread_mutex_t OnceEpochMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t OnceEpochCond = PTHREAD_COND_INITIALIZER;

namespace {
pthread_key_t EpochTSSKey;
pthread_once_t EpochTSSKeyFlag = PTHREAD_ONCE_INIT;

extern "C" {
static void DeleteEpochTSSData(void *Data) { free(Data); }

static void CreateEpochTSSData() {
  VERIFY_PTHREAD_CALL(pthread_key_create(&EpochTSSKey, &DeleteEpochTSSData));
}
}
} // namespace

AtomicT &GetOncePerThreadEpoch() {
  VERIFY_PTHREAD_CALL(pthread_once(&EpochTSSKeyFlag, &CreateEpochTSSData));
  void *Data = pthread_getspecific(EpochTSSKey);
  if (!Data) {
    Data = malloc(sizeof(AtomicT));
    if (!Data)
      Exception::ThrowSystemError("malloc");
    VERIFY_PTHREAD_CALL(pthread_setspecific(EpochTSSKey, Data));
    *static_cast<AtomicT *>(Data) = ~0u;
  }
  return *static_cast<AtomicT *>(Data);
}
} // namespace proxy
