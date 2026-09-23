#pragma once

#include "engine/physics/PhysicsTypes.h"
#include "engine/core/MathTypes.h"

namespace Engine::Physics {

struct RigidBodyComponent {
    MotionType motionType{MotionType::Dynamic};
    float mass{1.0f};
    float invMass{1.0f};

    Core::Vec3 linearVelocity{0.0f, 0.0f, 0.0f};
    Core::Vec3 angularVelocity{0.0f, 0.0f, 0.0f};

    Core::Vec3 forceAccumulator{0.0f, 0.0f, 0.0f};
    Core::Vec3 torqueAccumulator{0.0f, 0.0f, 0.0f};

    float linearDamping{0.05f};
    float angularDamping{0.05f};

    bool useGravity{true};
    bool isSleeping{false};

    // Sub-frame interpolation buffers
    Core::Vec3 previousPosition{0.0f, 0.0f, 0.0f};
    Core::Vec3 currentPosition{0.0f, 0.0f, 0.0f};

    void SetMass(float m) noexcept {
        mass = m;
        invMass = (motionType == MotionType::Dynamic && m > 1e-6f) ? (1.0f / m) : 0.0f;
    }

    void ApplyForce(const Core::Vec3& force) noexcept {
        if (motionType == MotionType::Dynamic) {
            forceAccumulator = forceAccumulator + force;
            isSleeping = false;
        }
    }

    void ApplyImpulse(const Core::Vec3& impulse) noexcept {
        if (motionType == MotionType::Dynamic) {
            linearVelocity = linearVelocity + impulse * invMass;
            isSleeping = false;
        }
    }

    void ClearForces() noexcept {
        forceAccumulator = {0.0f, 0.0f, 0.0f};
        torqueAccumulator = {0.0f, 0.0f, 0.0f};
    }
};

} // namespace Engine::Physics
