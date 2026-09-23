#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/world/Entity.h"
#include <string>

namespace Engine::World {

struct TagComponent {
    std::string tag{"Entity"};
};

struct TransformComponent {
    Core::Vec3 position{0.0f, 0.0f, 0.0f};
    Core::Vec3 rotation{0.0f, 0.0f, 0.0f}; // Euler angles (radians)
    Core::Vec3 scale{1.0f, 1.0f, 1.0f};

    Core::Mat4x4 localMatrix{Core::Mat4x4::Identity()};
    Core::Mat4x4 worldMatrix{Core::Mat4x4::Identity()};

    EntityID parent{NULL_ENTITY};
    EntityID firstChild{NULL_ENTITY};
    EntityID nextSibling{NULL_ENTITY};
    EntityID prevSibling{NULL_ENTITY};

    bool isDirty{true};

    void UpdateLocalMatrix() noexcept {
        localMatrix = Core::Mat4x4::Transform(position, rotation, scale);
    }
};

struct MeshComponent {
    Core::u32 meshIndex{0};
    Core::u32 materialIndex{0};
    bool isVisible{true};
    bool castShadows{true};
    Core::AABB localAABB{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    Core::AABB worldAABB{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
};

enum class LightType : Core::u8 {
    Directional = 0,
    Point = 1,
    Spot = 2
};

struct LightComponent {
    LightType type{LightType::Directional};
    float color[3]{1.0f, 1.0f, 1.0f};
    float intensity{1.0f};
    float range{15.0f};
    float innerSpotAngle{0.5f};
    float outerSpotAngle{0.785f};
};

struct CameraComponent {
    float fovY{0.785398f};
    float nearZ{0.1f};
    float farZ{1000.0f};
    bool isPrimary{true};
};

} // namespace Engine::World
