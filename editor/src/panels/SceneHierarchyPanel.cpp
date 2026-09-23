#include "editor/panels/SceneHierarchyPanel.h"
#include "engine/world/Components.h"
#include "engine/physics/ColliderComponent.h"
#include "engine/physics/RigidBodyComponent.h"
#include "engine/core/Log.h"

namespace Engine::Editor {

SceneHierarchyPanel::SceneHierarchyPanel()
    : EditorPanel("Scene Hierarchy") {
}

World::Entity SceneHierarchyPanel::CreateEmptyEntity(EditorContext& context, const std::string& name) {
    ENGINE_ASSERT(context.scene != nullptr, "Active scene must be valid");
    World::Entity e = context.scene->CreateEntity(name);
    context.SelectEntity(e.GetID());
    return e;
}

World::Entity SceneHierarchyPanel::CreateCubeEntity(EditorContext& context, const std::string& name) {
    World::Entity e = CreateEmptyEntity(context, name);
    e.AddComponent<World::MeshComponent>();

    auto& col = e.AddComponent<Physics::ColliderComponent>();
    col.type = Physics::ColliderType::Box;
    col.boxHalfExtents = {0.5f, 0.5f, 0.5f};

    auto& rb = e.AddComponent<Physics::RigidBodyComponent>();
    rb.motionType = Physics::MotionType::Dynamic;
    rb.SetMass(1.0f);

    return e;
}

World::Entity SceneHierarchyPanel::CreateLightEntity(EditorContext& context, const std::string& name) {
    World::Entity e = CreateEmptyEntity(context, name);
    auto& light = e.AddComponent<World::LightComponent>();
    light.type = World::LightType::Directional;
    light.intensity = 1.0f;
    return e;
}

World::Entity SceneHierarchyPanel::CreateCameraEntity(EditorContext& context, const std::string& name) {
    World::Entity e = CreateEmptyEntity(context, name);
    auto& cam = e.AddComponent<World::CameraComponent>();
    cam.isPrimary = true;
    return e;
}

void SceneHierarchyPanel::DeleteSelectedEntity(EditorContext& context) {
    if (context.HasSelection()) {
        context.scene->DestroyEntity(context.selectedEntity);
        context.Deselect();
    }
}

void SceneHierarchyPanel::ReparentEntity(EditorContext& context, World::EntityID child, World::EntityID newParent) {
    if (!context.scene) return;
    if (newParent != World::NULL_ENTITY) {
        context.scene->GetHierarchy().AttachChild(newParent, child, context.scene->GetRegistry());
    } else {
        context.scene->GetHierarchy().DetachChild(child, context.scene->GetRegistry());
    }
}

void SceneHierarchyPanel::RenderEntityNode(World::EntityID entity, EditorContext& context) {
    auto& registry = context.scene->GetRegistry();
    std::string tag = "Entity";
    if (registry.Has<World::TagComponent>(entity)) {
        tag = registry.Get<World::TagComponent>(entity).tag;
    }

    const bool isSelected = (context.selectedEntity == entity);
    m_uiLogs.push_back(std::format("Node: [{}] {} (Selected: {})", entity, tag, isSelected));
}

void SceneHierarchyPanel::OnRender(EditorContext& context) {
    m_uiLogs.clear();
    if (!context.scene) {
        m_uiLogs.push_back("No active scene loaded.");
        return;
    }

    auto& registry = context.scene->GetRegistry();
    if (!registry.HasPool<World::TransformComponent>()) {
        m_uiLogs.push_back("Scene empty (0 entities).");
        return;
    }

    const auto& pool = registry.GetPool<World::TransformComponent>();
    const auto& entities = pool.GetEntities();

    m_uiLogs.push_back(std::format("--- Hierarchy ({} Entities) ---", entities.size()));

    for (World::EntityID e : entities) {
        const auto& t = pool.Get(e);
        if (t.parent == World::NULL_ENTITY) {
            RenderEntityNode(e, context);
        }
    }
}

} // namespace Engine::Editor
