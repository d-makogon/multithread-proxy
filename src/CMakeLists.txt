set(HEADERS_LIST "${proxy_SOURCE_DIR}/include/Net/ServerSocket.hpp"
                "${proxy_SOURCE_DIR}/include/Net/Socket.hpp"
                "${proxy_SOURCE_DIR}/include/Net/Poller.hpp"
                "${proxy_SOURCE_DIR}/include/Net/PollHandlerBase.hpp"
                "${proxy_SOURCE_DIR}/include/Net/Server.hpp"
                "${proxy_SOURCE_DIR}/include/Net/ClientHandler.hpp"
                "${proxy_SOURCE_DIR}/include/Net/EndToEndHandlerBase.hpp"
                "${proxy_SOURCE_DIR}/include/Net/RemoteHandler.hpp"
                "${proxy_SOURCE_DIR}/include/Net/SocketBase.hpp"
                "${proxy_SOURCE_DIR}/include/Net/ServerHandler.hpp"
                "${proxy_SOURCE_DIR}/include/Cache/CacheBlock.hpp"
                "${proxy_SOURCE_DIR}/include/Cache/CacheRecord.hpp"
                "${proxy_SOURCE_DIR}/include/Cache/Cache.hpp"
                "${proxy_SOURCE_DIR}/include/Common/Globals.hpp"
                "${proxy_SOURCE_DIR}/include/Common/Utils.hpp"
                "${proxy_SOURCE_DIR}/include/Functional/TupleIndices.hpp"
                "${proxy_SOURCE_DIR}/include/Functional/Invoke.hpp"
                "${proxy_SOURCE_DIR}/include/Functional/Function.hpp"
                "${proxy_SOURCE_DIR}/include/Logging/Logger.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/Thread.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/Mutex.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/Condition.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/Once.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/PthreadHelpers.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/ThreadInterruptedException.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/ThreadDataBase.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/ThreadData.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/ThreadPool.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/LockGuard.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/Semaphore.hpp"
                "${proxy_SOURCE_DIR}/include/Parallel/ReadWriteLock.hpp"
                )

add_library(proxy_library Net/ServerSocket.cpp
                          Net/SocketBase.cpp
                          Net/Socket.cpp
                          Net/Poller.cpp
                          Net/Server.cpp
                          Net/ClientHandler.cpp
                          Net/RemoteHandler.cpp
                          Net/ServerHandler.cpp
                          Cache/CacheListener.cpp
                          Cache/CacheBlock.cpp
                          Cache/CacheRecord.cpp
                          Cache/Cache.cpp
                          Common/Utils.cpp
                          Logging/Logger.cpp
                          Parallel/Thread.cpp
                          Parallel/Mutex.cpp
                          Parallel/Once.cpp
                          Parallel/Condition.cpp
                          Parallel/Semaphore.cpp
                          Parallel/ReadWriteLock.cpp
                          Parallel/ThreadDataBase.cpp
                          ${HEADERS_LIST})

# Link pthread
# set(THREADS_PREFER_PTHREAD_FLAG ON)
# find_package(Threads REQUIRED)
# target_link_libraries(proxy_library PRIVATE Threads::Threads)
if(CMAKE_HOST_SYSTEM_NAME MATCHES "SunOS.*")
    message("Linking Solaris libraries")
    target_link_libraries(proxy_library nsl socket)
endif()


find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(proxy_library PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(proxy_library "${CMAKE_THREAD_LIBS_INIT}")
endif()

if (${CMAKE_VERSION} VERSION_GREATER "3.1")
  set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()

target_include_directories(proxy_library PUBLIC ../include)
target_include_directories(proxy_library PUBLIC ../httpparser/src)
