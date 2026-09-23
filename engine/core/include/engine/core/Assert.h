#pragma once

#include "engine/core/BaseTypes.h"
#include <string_view>

#if defined(_MSC_VER)
    #define ENGINE_DEBUG_BREAK() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__)
    #define ENGINE_DEBUG_BREAK() __builtin_trap()
#else
    #define ENGINE_DEBUG_BREAK() ((void)0)
#endif

namespace Engine::Core {

void ReportAssertionFailure(
    std::string_view expression,
    std::string_view message,
    std::string_view file,
    u32 line
);

} // namespace Engine::Core

#if defined(_DEBUG) || !defined(NDEBUG)
    #define ENGINE_ASSERT(expression, message) \
        do { \
            if (!(expression)) { \
                ::Engine::Core::ReportAssertionFailure(#expression, message, __FILE__, static_cast<::Engine::Core::u32>(__LINE__)); \
                ENGINE_DEBUG_BREAK(); \
            } \
        } while (false)
#else
    #define ENGINE_ASSERT(expression, message) ((void)0)
#endif

#define ENGINE_VERIFY(expression, message) \
    do { \
        if (!(expression)) { \
            ::Engine::Core::ReportAssertionFailure(#expression, message, __FILE__, static_cast<::Engine::Core::u32>(__LINE__)); \
            ENGINE_DEBUG_BREAK(); \
        } \
    } while (false)
