#pragma once

#include <cstdint>
#include <cstddef>

namespace Engine::Core {

// Fixed-width unsigned integer types
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// Fixed-width signed integer types
using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

// Floating point types
using f32 = float;
using f64 = double;

// Byte size constants
inline constexpr u64 KB = 1024ULL;
inline constexpr u64 MB = 1024ULL * KB;
inline constexpr u64 GB = 1024ULL * MB;

// Hardware cache line size standard
inline constexpr size_t CACHE_LINE_SIZE = 64;

// Memory alignment helpers
[[nodiscard]] constexpr bool IsPowerOfTwo(size_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

[[nodiscard]] constexpr size_t AlignUp(size_t value, size_t alignment) noexcept {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

[[nodiscard]] inline void* AlignPointer(void* ptr, size_t alignment) noexcept {
    const auto intPtr = reinterpret_cast<uintptr_t>(ptr);
    const auto aligned = (intPtr + (alignment - 1)) & ~(alignment - 1);
    return reinterpret_cast<void*>(aligned);
}

} // namespace Engine::Core
