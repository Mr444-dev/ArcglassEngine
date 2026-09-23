#pragma once

#include "editor/EditorPanel.h"
#include "editor/GizmoController.h"
#include "engine/renderer/Camera.h"

namespace Engine::Editor {

class ViewportPanel : public EditorPanel {
public:
    ViewportPanel();
    ~ViewportPanel() override = default;

    void OnRender(EditorContext& context) override;

    void SetViewportSize(Core::u32 width, Core::u32 height) noexcept {
        m_width = width;
        m_height = height;
    }

    [[nodiscard]] Core::u32 GetWidth() const noexcept { return m_width; }
    [[nodiscard]] Core::u32 GetHeight() const noexcept { return m_height; }
    [[nodiscard]] float GetAspectRatio() const noexcept {
        return (m_height > 0) ? (static_cast<float>(m_width) / static_cast<float>(m_height)) : (16.0f / 9.0f);
    }

    void OnPlayClicked(EditorContext& context) noexcept;
    void OnPauseClicked(EditorContext& context) noexcept;
    void OnStopClicked(EditorContext& context) noexcept;

    void OnGizmoModeSelected(EditorContext& context, GizmoMode mode) noexcept;

    [[nodiscard]] Renderer::Camera& GetEditorCamera() noexcept { return m_editorCamera; }
    [[nodiscard]] const Renderer::Camera& GetEditorCamera() const noexcept { return m_editorCamera; }

private:
    Core::u32 m_width{1280};
    Core::u32 m_height{720};
    Renderer::Camera m_editorCamera;
};

} // namespace Engine::Editor
