#include <Common/Utils.hpp>
#include <Parallel/ThreadDataBase.hpp>

namespace proxy {
void ThreadDataBase::BlockInterruptionSignals() {
  Utils::BlockInterruptionSignals();
}
} // namespace proxy
