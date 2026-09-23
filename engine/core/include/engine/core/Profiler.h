#pragma once

#include "engine/core/BaseTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace Engine::Core {

struct VRAMTracker {
    size_t staticBuffersBytes{0};
    size_t texturesBytes{0};
    size_t transientTargetsBytes{0};

    [[nodiscard]] size_t GetTotalBytes() const noexcept {
        return staticBuffersBytes + texturesBytes + transientTargetsBytes;
    }

    [[nodiscard]] double GetTotalMB() const noexcept {
        return static_cast<double>(GetTotalBytes()) / (1024.0 * 1024.0);
    }

    void AllocateStatic(size_t bytes) noexcept { staticBuffersBytes += bytes; }
    void FreeStatic(size_t bytes) noexcept { staticBuffersBytes = (staticBuffersBytes > bytes) ? (staticBuffersBytes - bytes) : 0; }

    void AllocateTexture(size_t bytes) noexcept { texturesBytes += bytes; }
    void FreeTexture(size_t bytes) noexcept { texturesBytes = (texturesBytes > bytes) ? (texturesBytes - bytes) : 0; }

    void AllocateTransient(size_t bytes) noexcept { transientTargetsBytes += bytes; }
    void FreeTransient(size_t bytes) noexcept { transientTargetsBytes = (transientTargetsBytes > bytes) ? (transientTargetsBytes - bytes) : 0; }
};

struct FrameTelemetry {
    float frameTimeMs{16.67f};
    float fps{60.0f};
    float cpuTimeMs{0.0f};
    float gpuTimeMs{0.0f};

    u32 drawCalls{0};
    u32 instancesTotal{0};
    u32 instancesVisible{0};
    u32 instancesCulled{0};
    u32 trianglesRendered{0};

    VRAMTracker vram;
};

class Profiler {
public:
    static Profiler& Get() noexcept;
    static void Initialize() noexcept {}
    static void Shutdown() noexcept {}

    void RecordScope(const std::string& name, double durationMs);
    void EndFrame(float deltaTime);

    [[nodiscard]] const FrameTelemetry& GetTelemetry() const noexcept { return m_telemetry; }
    [[nodiscard]] FrameTelemetry& GetTelemetry() noexcept { return m_telemetry; }
    [[nodiscard]] VRAMTracker& GetVRAM() noexcept { return m_telemetry.vram; }
    [[nodiscard]] const VRAMTracker& GetVRAM() const noexcept { return m_telemetry.vram; }

    [[nodiscard]] double GetScopeTime(const std::string& name) const noexcept;
    [[nodiscard]] const std::unordered_map<std::string, double>& GetAllScopeTimes() const noexcept {
        return m_scopeTimes;
    }

private:
    Profiler() = default;
    ~Profiler() = default;

    FrameTelemetry m_telemetry;
    std::unordered_map<std::string, double> m_scopeTimes;
};

class ProfileScope {
public:
    explicit ProfileScope(std::string name)
        : m_name(std::move(name)), m_start(std::chrono::high_resolution_clock::now()) {}

    ~ProfileScope() {
        const auto end = std::chrono::high_resolution_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(end - m_start).count();
        Profiler::Get().RecordScope(m_name, ms);
    }

private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
};

#define ENGINE_PROFILE_SCOPE(name) ::Engine::Core::ProfileScope __scope_##__LINE__(name)
#define ENGINE_PROFILE_ALLOC(ptr, size) do { (void)(ptr); (void)(size); } while(0)
#define ENGINE_PROFILE_FREE(ptr) do { (void)(ptr); } while(0)

} // namespace Engine::Core
