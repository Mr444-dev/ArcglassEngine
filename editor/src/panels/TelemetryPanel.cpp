#include "editor/panels/TelemetryPanel.h"
#include "engine/core/Profiler.h"
#include "engine/renderer/RenderDocCapture.h"
#include <format>

namespace Engine::Editor {

TelemetryPanel::TelemetryPanel()
    : EditorPanel("Telemetry & Profiler") {
}

void TelemetryPanel::OnRenderDocCaptureClicked() noexcept {
    Renderer::RenderDocCapture::Get().TriggerCapture();
}

void TelemetryPanel::OnRender(EditorContext& context) {
    (void)context;
    m_telemetryLogs.clear();

    const auto& t = Core::Profiler::Get().GetTelemetry();
    const auto& rdoc = Renderer::RenderDocCapture::Get();

    m_telemetryLogs.push_back(std::format("--- Frame Performance ---"));
    m_telemetryLogs.push_back(std::format("FPS: {:.1f} | Frame Time: {:.2f} ms", t.fps, t.frameTimeMs));
    m_telemetryLogs.push_back(std::format("CPU Time: {:.2f} ms | GPU Time: {:.2f} ms", t.cpuTimeMs, t.gpuTimeMs));

    m_telemetryLogs.push_back(std::format("--- Geometry & Culling Stats ---"));
    m_telemetryLogs.push_back(std::format("Draw Calls: {} | Triangles: {}", t.drawCalls, t.trianglesRendered));
    m_telemetryLogs.push_back(std::format("Instances: {} (Visible: {}, Culled: {})",
        t.instancesTotal, t.instancesVisible, t.instancesCulled));

    m_telemetryLogs.push_back(std::format("--- VRAM Budget Allocation ---"));
    m_telemetryLogs.push_back(std::format("Static Buffers: {:.2f} MB", static_cast<double>(t.vram.staticBuffersBytes) / (1024.0 * 1024.0)));
    m_telemetryLogs.push_back(std::format("Textures: {:.2f} MB", static_cast<double>(t.vram.texturesBytes) / (1024.0 * 1024.0)));
    m_telemetryLogs.push_back(std::format("Transient Targets: {:.2f} MB", static_cast<double>(t.vram.transientTargetsBytes) / (1024.0 * 1024.0)));
    m_telemetryLogs.push_back(std::format("Total VRAM: {:.2f} MB", t.vram.GetTotalMB()));

    m_telemetryLogs.push_back(std::format("--- Debugger & Tools ---"));
    m_telemetryLogs.push_back(std::format("RenderDoc API: {} (Captures: {})",
        rdoc.IsAvailable() ? "ATTACHED" : "NOT ATTACHED", rdoc.GetCaptureCount()));
}

} // namespace Engine::Editor
