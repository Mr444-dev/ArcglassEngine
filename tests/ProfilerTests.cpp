#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/core/Profiler.h"
#include "engine/renderer/RenderDocCapture.h"
#include "editor/EditorContext.h"
#include "editor/panels/TelemetryPanel.h"
#include <thread>
#include <chrono>

void TestProfileScopeAndAggregation() {
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing RAII ProfileScope & Timing Aggregation ---");

    Profiler& profiler = Profiler::Get();

    {
        ENGINE_PROFILE_SCOPE("SimulationTick");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    const double scopeMs = profiler.GetScopeTime("SimulationTick");
    ENGINE_ASSERT(scopeMs > 0.0, "ProfileScope did not record a positive duration");
    ENGINE_LOG_INFO("Scope 'SimulationTick' recorded: {:.2f} ms.", scopeMs);

    constexpr float expectedFrameSeconds = 1.0f / 60.0f;
    profiler.EndFrame(expectedFrameSeconds);
    const auto& t = profiler.GetTelemetry();
    ENGINE_ASSERT(std::abs(t.fps - 60.0f) < 0.1f, "FrameTelemetry FPS calculation mismatch");
    ENGINE_ASSERT(t.cpuTimeMs > 0.0f, "CPU time must include recorded scopes");

    ENGINE_LOG_INFO("ProfileScope and Telemetry aggregation passed.");
}

void TestVRAMTrackerAllocation() {
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing VRAM Memory Tracker ---");

    Profiler& profiler = Profiler::Get();
    auto& vram = profiler.GetVRAM();

    // Reset
    vram.staticBuffersBytes = 0;
    vram.texturesBytes = 0;
    vram.transientTargetsBytes = 0;

    // Allocate 10 MB static, 20 MB textures, 5 MB transient
    vram.AllocateStatic(10 * 1024 * 1024);
    vram.AllocateTexture(20 * 1024 * 1024);
    vram.AllocateTransient(5 * 1024 * 1024);

    ENGINE_ASSERT(vram.GetTotalBytes() == 35 * 1024 * 1024, "Total VRAM bytes mismatch");
    ENGINE_ASSERT(std::abs(vram.GetTotalMB() - 35.0) < 1e-3, "Total VRAM MB mismatch");

    // Free transient targets
    vram.FreeTransient(5 * 1024 * 1024);
    ENGINE_ASSERT(vram.transientTargetsBytes == 0, "Transient VRAM should be zero after free");
    ENGINE_ASSERT(std::abs(vram.GetTotalMB() - 30.0) < 1e-3, "VRAM after free mismatch");

    ENGINE_LOG_INFO("VRAM Tracker verified (Allocated 35 MB, Freed 5 MB transient, 30 MB static remaining).");
}

void TestRenderDocAPIProgrammaticCapture() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 9: RenderDoc Programmatic API Hooking ---");

    RenderDocCapture& rdoc = RenderDocCapture::Get();
    rdoc.Initialize();

    // Trigger capture
    rdoc.TriggerCapture();

    // Emulate frame boundary
    rdoc.StartFrameCapture(nullptr, nullptr);
    rdoc.EndFrameCapture(nullptr, nullptr);

    ENGINE_LOG_INFO("RenderDoc capture pipeline tested (Status: {}, Captures: {}).",
        rdoc.IsAvailable() ? "ATTACHED" : "SAFE FALLBACK", rdoc.GetCaptureCount());
}

void TestTelemetryPanelRendering() {
    using namespace Engine::Editor;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing TelemetryPanel Formatting ---");

    EditorContext context;
    TelemetryPanel panel;

    // Simulate telemetry values
    auto& t = Profiler::Get().GetTelemetry();
    t.drawCalls = 42;
    t.instancesTotal = 100000;
    t.instancesVisible = 49912;
    t.instancesCulled = 50088;
    t.trianglesRendered = 250000;

    panel.OnRender(context);
    const auto& logs = panel.GetTelemetryLogs();
    ENGINE_ASSERT(!logs.empty(), "Telemetry panel logs should not be empty");

    panel.OnRenderDocCaptureClicked();

    ENGINE_LOG_INFO("TelemetryPanel verified ({} metric lines rendered).", logs.size());
}

void RunProfilerUnitTests() {
    TestProfileScopeAndAggregation();
    TestVRAMTrackerAllocation();
    TestRenderDocAPIProgrammaticCapture();
    TestTelemetryPanelRendering();
}
