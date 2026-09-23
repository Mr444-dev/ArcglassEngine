#include "editor/panels/ViewportPanel.h"
#include "engine/core/Log.h"

namespace Engine::Editor {

ViewportPanel::ViewportPanel()
    : EditorPanel("Viewport") {
    m_editorCamera.SetPosition({0.0f, 5.0f, -10.0f});
    m_editorCamera.SetTarget({0.0f, 0.0f, 0.0f});
    m_editorCamera.SetLens(0.785398f, GetAspectRatio(), 0.1f, 1000.0f);
    m_editorCamera.UpdateMatrices();
}

void ViewportPanel::OnPlayClicked(EditorContext& context) noexcept {
    context.isPlaying = true;
    context.isPaused = false;
    ENGINE_LOG_INFO("Editor: Simulation STARTED.");
}

void ViewportPanel::OnPauseClicked(EditorContext& context) noexcept {
    if (context.isPlaying) {
        context.isPaused = !context.isPaused;
        ENGINE_LOG_INFO("Editor: Simulation {}.", context.isPaused ? "PAUSED" : "RESUMED");
    }
}

void ViewportPanel::OnStopClicked(EditorContext& context) noexcept {
    context.isPlaying = false;
    context.isPaused = false;
    ENGINE_LOG_INFO("Editor: Simulation STOPPED.");
}

void ViewportPanel::OnGizmoModeSelected(EditorContext& context, GizmoMode mode) noexcept {
    context.gizmoMode = mode;
}

void ViewportPanel::OnRender(EditorContext& context) {
    context.viewportWidth = m_width;
    context.viewportHeight = m_height;

    // Update camera lens when aspect ratio changes
    m_editorCamera.SetLens(0.785398f, GetAspectRatio(), 0.1f, 1000.0f);
    m_editorCamera.UpdateMatrices();
}

} // namespace Engine::Editor
