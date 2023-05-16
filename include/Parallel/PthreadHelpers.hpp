#pragma once

#include <Common/ProxyException.hpp>
#include <cstring>
#include <pthread.h>

namespace proxy {
#define VERIFY_PTHREAD_CALL(call)                                              \
  do {                                                                         \
    int status = call;                                                         \
    if (status != 0)                                                           \
      Exception::ThrowSystemError(status);                                     \
  } while (0)

#define LOCK_MUTEX(m) VERIFY_PTHREAD_CALL(pthread_mutex_lock(m))
#define UNLOCK_MUTEX(m) VERIFY_PTHREAD_CALL(pthread_mutex_unlock(m))
#define COND_WAIT(c, m) VERIFY_PTHREAD_CALL(pthread_cond_wait(c, m))
#define COND_BROADCAST(c) VERIFY_PTHREAD_CALL(pthread_cond_broadcast(c))
} // namespace proxy
