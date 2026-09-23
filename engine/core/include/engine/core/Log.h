#pragma once

#include "engine/core/BaseTypes.h"
#include <string_view>
#include <format>

namespace Engine::Core {

enum class LogLevel : u8 {
    Trace,
    Info,
    Warn,
    Error,
    Fatal
};

class Logger {
public:
    static void LogMessage(LogLevel level, std::string_view message);

    template <typename... Args>
    static void Log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        std::string formatted = std::format(fmt, std::forward<Args>(args)...);
        LogMessage(level, formatted);
    }
};

} // namespace Engine::Core

#define ENGINE_LOG_TRACE(...) ::Engine::Core::Logger::Log(::Engine::Core::LogLevel::Trace, __VA_ARGS__)
#define ENGINE_LOG_INFO(...)  ::Engine::Core::Logger::Log(::Engine::Core::LogLevel::Info,  __VA_ARGS__)
#define ENGINE_LOG_WARN(...)  ::Engine::Core::Logger::Log(::Engine::Core::LogLevel::Warn,  __VA_ARGS__)
#define ENGINE_LOG_ERROR(...) ::Engine::Core::Logger::Log(::Engine::Core::LogLevel::Error, __VA_ARGS__)
#define ENGINE_LOG_FATAL(...) ::Engine::Core::Logger::Log(::Engine::Core::LogLevel::Fatal, __VA_ARGS__)
