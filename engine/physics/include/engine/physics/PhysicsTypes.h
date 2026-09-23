#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/world/Entity.h"

namespace Engine::Physics {

enum class MotionType : Core::u8 {
    Static = 0,    // Immovable environment (infinite mass)
    Kinematic = 1, // Controlled by script/animation directly
    Dynamic = 2    // Simulated by physics engine (forces, impulses, gravity)
};

enum class ColliderType : Core::u8 {
    Sphere = 0,
    Box = 1,
    Capsule = 2,
    Plane = 3
};

struct ContactPoint {
    Core::Vec3 position{0.0f, 0.0f, 0.0f};
    Core::Vec3 normal{0.0f, 1.0f, 0.0f};
    float penetrationDepth{0.0f};
};

struct ContactManifold {
    World::EntityID entityA{World::NULL_ENTITY};
    World::EntityID entityB{World::NULL_ENTITY};
    ContactPoint contact;
    bool hasContact{false};
};

struct Ray {
    Core::Vec3 origin{0.0f, 0.0f, 0.0f};
    Core::Vec3 direction{0.0f, 0.0f, 1.0f};
    float maxDistance{1000.0f};
};

struct RaycastHit {
    World::EntityID entity{World::NULL_ENTITY};
    Core::Vec3 point{0.0f, 0.0f, 0.0f};
    Core::Vec3 normal{0.0f, 1.0f, 0.0f};
    float distance{0.0f};
    bool hit{false};
};

} // namespace Engine::Physics
