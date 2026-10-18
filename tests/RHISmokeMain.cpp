#include "engine/core/Log.h"

void RunRHIGraphicsTest();

int main() {
    Engine::Core::Logger::LogMessage(Engine::Core::LogLevel::Info, "Starting hardware-dependent Direct3D 12 smoke test.");
    RunRHIGraphicsTest();
    Engine::Core::Logger::LogMessage(Engine::Core::LogLevel::Info, "Direct3D 12 smoke test completed successfully.");
    return 0;
}
