// The used algorithm is based on Mike Burrows fast_pthread_once algorithm as
// described in
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2444.html
#pragma once

#include <Parallel/PthreadHelpers.hpp>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <pthread.h>

namespace proxy {
using AtomicT = std::uint32_t;

AtomicT &GetOncePerThreadEpoch();
extern AtomicT OnceGlobalEpoch;
extern pthread_mutex_t OnceEpochMutex;
extern pthread_cond_t OnceEpochCond;

struct OnceFlag {
  volatile AtomicT Epoch;
};

template <typename F> inline void CallOnce(OnceFlag &Flag, F &&Func) {
  static const AtomicT UninitializedFlag = 0;
  static const AtomicT BeingInitialized = UninitializedFlag + 1;
  const AtomicT Epoch = Flag.Epoch;
  AtomicT &ThisThreadEpoch = GetOncePerThreadEpoch();

  if (Epoch < ThisThreadEpoch) {
    LOCK_MUTEX(&OnceEpochMutex);
    while (Flag.Epoch <= BeingInitialized) {
      if (Flag.Epoch == UninitializedFlag) {
        Flag.Epoch = BeingInitialized;
        try {
          UNLOCK_MUTEX(&OnceEpochMutex);
          Func.Run();
        } catch (...) {
          LOCK_MUTEX(&OnceEpochMutex);
          Flag.Epoch = UninitializedFlag;
          COND_BROADCAST(&OnceEpochCond);
          UNLOCK_MUTEX(&OnceEpochMutex);
          throw;
        }
        LOCK_MUTEX(&OnceEpochMutex);
        Flag.Epoch = --OnceGlobalEpoch;
        COND_BROADCAST(&OnceEpochCond);
      } else
        while (Flag.Epoch == BeingInitialized)
          COND_WAIT(&OnceEpochCond, &OnceEpochMutex);
    }
    ThisThreadEpoch = OnceGlobalEpoch;
    UNLOCK_MUTEX(&OnceEpochMutex);
  }
}
} // namespace proxy
