#pragma once
#include <cstddef>

namespace proxy::Globals {
constexpr std::size_t DefaultReadBufferSize{4096};
constexpr std::size_t DefaultCacheBlockSize{4096};
constexpr std::size_t DefaultResponseBufferSize{4096};
constexpr std::size_t StackBufferSize{2048};
constexpr std::size_t BufferConcatSizeThreshold{
    static_cast<std::size_t>(DefaultCacheBlockSize * 0.2)};
constexpr std::size_t ClientTimeoutSec{666};
constexpr std::size_t ClientTimeoutMSec{ClientTimeoutSec * 1000};
} // namespace proxy::Globals
