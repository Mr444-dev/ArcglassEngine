#include "engine/core/Profiler.h"

namespace Engine::Core {

Profiler& Profiler::Get() noexcept {
    static Profiler instance;
    return instance;
}

void Profiler::RecordScope(const std::string& name, double durationMs) {
    m_scopeTimes[name] = durationMs;
}

void Profiler::EndFrame(float deltaTime) {
    m_telemetry.frameTimeMs = deltaTime * 1000.0f;
    m_telemetry.fps = (deltaTime > 1e-6f) ? (1.0f / deltaTime) : 60.0f;

    // Sum CPU timing contributions
    double totalCpu = 0.0;
    for (const auto& [name, time] : m_scopeTimes) {
        totalCpu += time;
    }
    m_telemetry.cpuTimeMs = static_cast<float>(totalCpu);
}

double Profiler::GetScopeTime(const std::string& name) const noexcept {
    auto it = m_scopeTimes.find(name);
    return (it != m_scopeTimes.end()) ? it->second : 0.0;
}

} // namespace Engine::Core
