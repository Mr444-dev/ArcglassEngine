#include "editor/panels/PropertiesPanel.h"
#include "engine/world/Components.h"
#include "engine/physics/ColliderComponent.h"
#include "engine/physics/RigidBodyComponent.h"
#include <format>

namespace Engine::Editor {

PropertiesPanel::PropertiesPanel()
    : EditorPanel("Inspector") {
}

void PropertiesPanel::InspectComponents(World::EntityID entity, EditorContext& context) {
    auto& registry = context.scene->GetRegistry();

    // 1. Tag
    if (registry.Has<World::TagComponent>(entity)) {
        auto& tag = registry.Get<World::TagComponent>(entity);
        m_inspectorLogs.push_back(std::format("[TagComponent] Name: {}", tag.tag));
    }

    // 2. Transform
    if (registry.Has<World::TransformComponent>(entity)) {
        auto& t = registry.Get<World::TransformComponent>(entity);
        m_inspectorLogs.push_back(std::format("[TransformComponent] Pos: ({:.2f}, {:.2f}, {:.2f}) | Rot: ({:.2f}, {:.2f}, {:.2f}) | Scale: ({:.2f}, {:.2f}, {:.2f})",
            t.position.x, t.position.y, t.position.z,
            t.rotation.x, t.rotation.y, t.rotation.z,
            t.scale.x, t.scale.y, t.scale.z));
    }

    // 3. Mesh
    if (registry.Has<World::MeshComponent>(entity)) {
        auto& mesh = registry.Get<World::MeshComponent>(entity);
        m_inspectorLogs.push_back(std::format("[MeshComponent] MeshID: {} | MaterialID: {} | Visible: {}",
            mesh.meshIndex, mesh.materialIndex, mesh.isVisible));
    }

    // 4. Light
    if (registry.Has<World::LightComponent>(entity)) {
        auto& light = registry.Get<World::LightComponent>(entity);
        m_inspectorLogs.push_back(std::format("[LightComponent] Type: {} | Intensity: {:.2f} | Range: {:.2f}",
            static_cast<int>(light.type), light.intensity, light.range));
    }

    // 5. Camera
    if (registry.Has<World::CameraComponent>(entity)) {
        auto& cam = registry.Get<World::CameraComponent>(entity);
        m_inspectorLogs.push_back(std::format("[CameraComponent] FOV: {:.2f} rad | Near: {:.2f} | Far: {:.2f} | Primary: {}",
            cam.fovY, cam.nearZ, cam.farZ, cam.isPrimary));
    }

    // 6. RigidBody
    if (registry.Has<Physics::RigidBodyComponent>(entity)) {
        auto& rb = registry.Get<Physics::RigidBodyComponent>(entity);
        m_inspectorLogs.push_back(std::format("[RigidBodyComponent] Motion: {} | Mass: {:.2f} kg | Gravity: {}",
            static_cast<int>(rb.motionType), rb.mass, rb.useGravity));
    }

    // 7. Collider
    if (registry.Has<Physics::ColliderComponent>(entity)) {
        auto& col = registry.Get<Physics::ColliderComponent>(entity);
        m_inspectorLogs.push_back(std::format("[ColliderComponent] Type: {} | Restitution: {:.2f} | Friction: {:.2f}",
            static_cast<int>(col.type), col.restitution, col.friction));
    }
}

void PropertiesPanel::OnRender(EditorContext& context) {
    m_inspectorLogs.clear();

    if (!context.HasSelection()) {
        m_inspectorLogs.push_back("Select an entity in the hierarchy to inspect properties.");
        return;
    }

    m_inspectorLogs.push_back(std::format("--- Entity Inspector (ID: {}) ---", context.selectedEntity));
    InspectComponents(context.selectedEntity, context);
}

} // namespace Engine::Editor
