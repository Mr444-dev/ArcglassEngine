#pragma once

#include "editor/EditorPanel.h"
#include <vector>

namespace Engine::Editor {

class SceneHierarchyPanel : public EditorPanel {
public:
    SceneHierarchyPanel();
    ~SceneHierarchyPanel() override = default;

    void OnRender(EditorContext& context) override;

    // Helper functions for entity operations
    World::Entity CreateEmptyEntity(EditorContext& context, const std::string& name = "Entity");
    World::Entity CreateCubeEntity(EditorContext& context, const std::string& name = "Cube");
    World::Entity CreateLightEntity(EditorContext& context, const std::string& name = "DirectionalLight");
    World::Entity CreateCameraEntity(EditorContext& context, const std::string& name = "MainCamera");

    void DeleteSelectedEntity(EditorContext& context);
    void ReparentEntity(EditorContext& context, World::EntityID child, World::EntityID newParent);

    [[nodiscard]] const std::vector<std::string>& GetRenderLogs() const noexcept { return m_uiLogs; }

private:
    void RenderEntityNode(World::EntityID entity, EditorContext& context);

    std::vector<std::string> m_uiLogs;
};

} // namespace Engine::Editor
