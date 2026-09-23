#pragma once

#include "editor/EditorContext.h"
#include "editor/panels/SceneHierarchyPanel.h"
#include "editor/panels/PropertiesPanel.h"
#include "editor/panels/AssetBrowserPanel.h"
#include "editor/panels/ViewportPanel.h"
#include "engine/world/Scene.h"
#include "engine/physics/PhysicsScene.h"
#include <memory>

namespace Engine::Editor {

class EditorApp {
public:
    EditorApp();
    ~EditorApp() = default;

    void Initialize();
    void Update(float deltaTime);

    [[nodiscard]] EditorContext& GetContext() noexcept { return m_context; }
    [[nodiscard]] SceneHierarchyPanel& GetHierarchyPanel() noexcept { return m_hierarchyPanel; }
    [[nodiscard]] PropertiesPanel& GetPropertiesPanel() noexcept { return m_propertiesPanel; }
    [[nodiscard]] AssetBrowserPanel& GetAssetBrowserPanel() noexcept { return m_assetBrowserPanel; }
    [[nodiscard]] ViewportPanel& GetViewportPanel() noexcept { return m_viewportPanel; }

private:
    std::unique_ptr<World::Scene> m_activeScene;
    std::unique_ptr<Physics::PhysicsScene> m_physicsScene;

    EditorContext m_context;

    SceneHierarchyPanel m_hierarchyPanel;
    PropertiesPanel m_propertiesPanel;
    AssetBrowserPanel m_assetBrowserPanel;
    ViewportPanel m_viewportPanel;
};

} // namespace Engine::Editor
