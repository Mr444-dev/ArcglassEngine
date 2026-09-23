#include "engine/core/Log.h"
#include <iostream>
#include <mutex>
#include <chrono>

namespace Engine::Core {

namespace {
std::mutex g_logMutex;

const char* GetLevelPrefix(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "[TRACE]";
        case LogLevel::Info:  return "[INFO] ";
        case LogLevel::Warn:  return "[WARN] ";
        case LogLevel::Error: return "[ERROR]";
        case LogLevel::Fatal: return "[FATAL]";
    }
    return "[LOG]  ";
}
} // namespace

void Logger::LogMessage(LogLevel level, std::string_view message) {
    if (level < LogLevel::Info) return;
    std::lock_guard<std::mutex> lock(g_logMutex);
    
    std::ostream& targetStream = (level >= LogLevel::Error) ? std::cerr : std::cout;
    targetStream << GetLevelPrefix(level) << " " << message << std::endl;
}

} // namespace Engine::Core
