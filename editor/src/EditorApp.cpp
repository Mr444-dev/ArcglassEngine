#include "editor/EditorApp.h"
#include "engine/assets/VirtualFileSystem.h"
#include "engine/core/Log.h"

namespace Engine::Editor {

EditorApp::EditorApp() {
    Assets::VirtualFileSystem::Get().Mount("assets", "./assets");
    Assets::VirtualFileSystem::Get().Mount("engine", "./engine");

    m_activeScene = std::make_unique<World::Scene>();
    m_physicsScene = std::make_unique<Physics::PhysicsScene>();

    m_context.scene = m_activeScene.get();
    m_context.physicsScene = m_physicsScene.get();
}

void EditorApp::Initialize() {
    ENGINE_LOG_INFO("EditorApp initialized with Dockspace and Panels.");

    // Create default starter scene
    m_hierarchyPanel.CreateCameraEntity(m_context, "MainCamera");
    m_hierarchyPanel.CreateLightEntity(m_context, "SunLight");
    m_hierarchyPanel.CreateCubeEntity(m_context, "DefaultCube");
}

void EditorApp::Update(float deltaTime) {
    // 1. If in Play Mode, update physics and simulation
    if (m_context.isPlaying && !m_context.isPaused) {
        if (m_physicsScene && m_activeScene) {
            m_physicsScene->Update(deltaTime, *m_activeScene);
        }
    }

    // 2. Update Scene Transforms and Spatial BVH
    if (m_activeScene) {
        m_activeScene->Update(deltaTime);
    }

    // 3. Render all docked panels
    m_viewportPanel.OnRender(m_context);
    m_hierarchyPanel.OnRender(m_context);
    m_propertiesPanel.OnRender(m_context);
    m_assetBrowserPanel.OnRender(m_context);
}

} // namespace Engine::Editor
