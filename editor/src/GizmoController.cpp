#include "editor/GizmoController.h"
#include "engine/world/Components.h"

namespace Engine::Editor {

void GizmoController::ApplyTranslation(EditorContext& context, const Core::Vec3& delta) {
    if (!context.HasSelection()) return;

    auto& registry = context.scene->GetRegistry();
    if (registry.Has<World::TransformComponent>(context.selectedEntity)) {
        auto& t = registry.Get<World::TransformComponent>(context.selectedEntity);
        t.position = t.position + delta;
        t.isDirty = true;
    }
}

void GizmoController::ApplyRotation(EditorContext& context, const Core::Vec3& deltaEuler) {
    if (!context.HasSelection()) return;

    auto& registry = context.scene->GetRegistry();
    if (registry.Has<World::TransformComponent>(context.selectedEntity)) {
        auto& t = registry.Get<World::TransformComponent>(context.selectedEntity);
        t.rotation = t.rotation + deltaEuler;
        t.isDirty = true;
    }
}

void GizmoController::ApplyScale(EditorContext& context, const Core::Vec3& deltaScale) {
    if (!context.HasSelection()) return;

    auto& registry = context.scene->GetRegistry();
    if (registry.Has<World::TransformComponent>(context.selectedEntity)) {
        auto& t = registry.Get<World::TransformComponent>(context.selectedEntity);
        t.scale.x *= deltaScale.x;
        t.scale.y *= deltaScale.y;
        t.scale.z *= deltaScale.z;
        t.isDirty = true;
    }
}

Core::Vec3 GizmoController::CalculateTranslationDelta(
    GizmoAxis axis,
    float mouseDeltaX,
    float mouseDeltaY,
    const Renderer::Camera& camera
) {
    (void)camera;
    constexpr float SENSITIVITY = 0.02f;

    switch (axis) {
        case GizmoAxis::X:
            return {mouseDeltaX * SENSITIVITY, 0.0f, 0.0f};
        case GizmoAxis::Y:
            return {0.0f, -mouseDeltaY * SENSITIVITY, 0.0f};
        case GizmoAxis::Z:
            return {0.0f, 0.0f, (mouseDeltaX - mouseDeltaY) * 0.5f * SENSITIVITY};
        case GizmoAxis::XYZ:
            return {mouseDeltaX * SENSITIVITY, -mouseDeltaY * SENSITIVITY, 0.0f};
        case GizmoAxis::None:
            break;
    }
    return {0.0f, 0.0f, 0.0f};
}

} // namespace Engine::Editor
