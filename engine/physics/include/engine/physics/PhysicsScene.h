#pragma once

#include "engine/physics/PhysicsTypes.h"
#include "engine/physics/ColliderComponent.h"
#include "engine/physics/RigidBodyComponent.h"
#include "engine/core/MathTypes.h"
#include "engine/world/Scene.h"
#include <vector>

namespace Engine::Physics {

class PhysicsScene {
public:
    PhysicsScene();
    ~PhysicsScene() = default;

    void Update(float deltaTime, World::Scene& worldScene);

    bool Raycast(const Ray& ray, RaycastHit& outHit, World::Scene& worldScene) const;

    void SetGravity(const Core::Vec3& g) noexcept { m_gravity = g; }
    [[nodiscard]] const Core::Vec3& GetGravity() const noexcept { return m_gravity; }

    void SetFixedDeltaTime(float dt) noexcept { m_fixedDeltaTime = dt; }
    [[nodiscard]] float GetFixedDeltaTime() const noexcept { return m_fixedDeltaTime; }
    [[nodiscard]] float GetInterpolationAlpha() const noexcept { return m_interpolationAlpha; }

    [[nodiscard]] size_t GetActiveCollisionCount() const noexcept { return m_activeContacts.size(); }

private:
    void StepSimulation(float dt, World::Scene& worldScene);
    void DetectCollisions(World::Scene& worldScene);
    void ResolveCollisions(World::Scene& worldScene, float dt);

    Core::Vec3 m_gravity{0.0f, -9.81f, 0.0f};
    float m_fixedDeltaTime{1.0f / 60.0f}; // Fixed 60 Hz
    float m_accumulator{0.0f};
    float m_interpolationAlpha{0.0f};

    std::vector<ContactManifold> m_activeContacts;
};

} // namespace Engine::Physics
