#include <Logging/Logger.hpp>

namespace proxy::Log {
DefaultLoggerT DefaultLogger = DefaultLoggerT(&std::cerr);
} // namespace proxy::Log
