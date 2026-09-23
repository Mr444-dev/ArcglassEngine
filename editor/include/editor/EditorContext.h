#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/world/Entity.h"
#include "engine/world/Scene.h"
#include "engine/physics/PhysicsScene.h"

namespace Engine::Editor {

enum class GizmoMode : Core::u8 {
    Translate = 0,
    Rotate = 1,
    Scale = 2
};

enum class TransformSpace : Core::u8 {
    World = 0,
    Local = 1
};

struct EditorContext {
    World::Scene* scene{nullptr};
    Physics::PhysicsScene* physicsScene{nullptr};

    World::EntityID selectedEntity{World::NULL_ENTITY};
    GizmoMode gizmoMode{GizmoMode::Translate};
    TransformSpace transformSpace{TransformSpace::World};

    bool isPlaying{false};
    bool isPaused{false};

    Core::u32 viewportWidth{1280};
    Core::u32 viewportHeight{720};
    bool viewportHovered{false};
    bool viewportFocused{false};

    void SelectEntity(World::EntityID entity) noexcept {
        selectedEntity = entity;
    }

    void Deselect() noexcept {
        selectedEntity = World::NULL_ENTITY;
    }

    [[nodiscard]] bool HasSelection() const noexcept {
        return selectedEntity != World::NULL_ENTITY && scene != nullptr;
    }
};

} // namespace Engine::Editor
