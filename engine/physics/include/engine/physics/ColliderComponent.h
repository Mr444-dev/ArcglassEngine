#pragma once

#include "engine/physics/PhysicsTypes.h"
#include "engine/core/MathTypes.h"

namespace Engine::Physics {

struct ColliderComponent {
    ColliderType type{ColliderType::Box};
    Core::Vec3 offset{0.0f, 0.0f, 0.0f};

    // Box dimensions
    Core::Vec3 boxHalfExtents{0.5f, 0.5f, 0.5f};

    // Sphere dimensions
    float sphereRadius{0.5f};

    // Capsule dimensions
    float capsuleRadius{0.4f};
    float capsuleHeight{1.8f};

    // Plane dimensions
    Core::Vec3 planeNormal{0.0f, 1.0f, 0.0f};
    float planeDistance{0.0f};

    bool isTrigger{false};
    float friction{0.5f};
    float restitution{0.2f};

    [[nodiscard]] Core::AABB ComputeWorldAABB(const Core::Vec3& worldPos) const noexcept {
        const Core::Vec3 center = worldPos + offset;
        switch (type) {
            case ColliderType::Box:
                return {center - boxHalfExtents, center + boxHalfExtents};
            case ColliderType::Sphere:
                return {
                    center - Core::Vec3{sphereRadius, sphereRadius, sphereRadius},
                    center + Core::Vec3{sphereRadius, sphereRadius, sphereRadius}
                };
            case ColliderType::Capsule: {
                const float halfH = capsuleHeight * 0.5f;
                return {
                    center - Core::Vec3{capsuleRadius, halfH, capsuleRadius},
                    center + Core::Vec3{capsuleRadius, halfH, capsuleRadius}
                };
            }
            case ColliderType::Plane:
                return {{-10000.0f, -0.1f, -10000.0f}, {10000.0f, 0.1f, 10000.0f}};
        }
        return {center - Core::Vec3{0.5f, 0.5f, 0.5f}, center + Core::Vec3{0.5f, 0.5f, 0.5f}};
    }
};

} // namespace Engine::Physics
