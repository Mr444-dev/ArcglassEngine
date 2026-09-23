#include "engine/physics/PhysicsScene.h"
#include "engine/physics/Collision.h"
#include "engine/core/Log.h"
#include <algorithm>

namespace Engine::Physics {

PhysicsScene::PhysicsScene() {
    ENGINE_LOG_INFO("PhysicsScene initialized (Fixed Timestep 60 Hz).");
}

void PhysicsScene::Update(float deltaTime, World::Scene& worldScene) {
    m_accumulator += deltaTime;

    // Prevent spiral of death if frame time spikes
    constexpr float MAX_ACCUMULATOR = 0.2f;
    if (m_accumulator > MAX_ACCUMULATOR) {
        m_accumulator = MAX_ACCUMULATOR;
    }

    while (m_accumulator >= m_fixedDeltaTime) {
        StepSimulation(m_fixedDeltaTime, worldScene);
        m_accumulator -= m_fixedDeltaTime;
    }

    m_interpolationAlpha = m_accumulator / m_fixedDeltaTime;
}

void PhysicsScene::StepSimulation(float dt, World::Scene& worldScene) {
    auto& registry = worldScene.GetRegistry();

    // 1. Force & Gravity Integration
    if (registry.HasPool<RigidBodyComponent>()) {
        auto& rbPool = registry.GetPool<RigidBodyComponent>();
        auto& rigidBodies = rbPool.GetData();
        const auto& entities = rbPool.GetEntities();

        for (size_t i = 0; i < rigidBodies.size(); ++i) {
            auto& rb = rigidBodies[i];
            const World::EntityID e = entities[i];

            if (rb.motionType != MotionType::Dynamic || rb.isSleeping) {
                continue;
            }

            if (registry.Has<World::TransformComponent>(e)) {
                auto& t = registry.Get<World::TransformComponent>(e);

                // Apply gravity
                if (rb.useGravity) {
                    rb.ApplyForce(m_gravity * rb.mass);
                }

                // Integrate linear velocity
                const Core::Vec3 acceleration = rb.forceAccumulator * rb.invMass;
                rb.linearVelocity = rb.linearVelocity + acceleration * dt;

                // Apply linear damping
                const float dampingFactor = std::clamp(1.0f - rb.linearDamping * dt, 0.0f, 1.0f);
                rb.linearVelocity = rb.linearVelocity * dampingFactor;

                // Cache previous position for render interpolation
                rb.previousPosition = t.position;

                // Integrate position
                t.position = t.position + rb.linearVelocity * dt;
                rb.currentPosition = t.position;

                // Clear accumulated forces
                rb.ClearForces();
            }
        }
    }

    // 2. Detect Collisions
    DetectCollisions(worldScene);

    // 3. Resolve Collisions (Impulses & Separation)
    ResolveCollisions(worldScene, dt);
}

void PhysicsScene::DetectCollisions(World::Scene& worldScene) {
    m_activeContacts.clear();

    auto& registry = worldScene.GetRegistry();
    if (!registry.HasPool<ColliderComponent>()) return;

    auto& colPool = registry.GetPool<ColliderComponent>();
    const auto& colliders = colPool.GetData();
    const auto& entities = colPool.GetEntities();
    const size_t count = colliders.size();

    for (size_t i = 0; i < count; ++i) {
        const World::EntityID eA = entities[i];
        const auto& colA = colliders[i];
        if (!registry.Has<World::TransformComponent>(eA)) continue;
        const auto& tA = registry.Get<World::TransformComponent>(eA);
        const Core::Vec3 posA = tA.position + colA.offset;

        for (size_t j = i + 1; j < count; ++j) {
            const World::EntityID eB = entities[j];
            const auto& colB = colliders[j];
            if (!registry.Has<World::TransformComponent>(eB)) continue;
            const auto& tB = registry.Get<World::TransformComponent>(eB);
            const Core::Vec3 posB = tB.position + colB.offset;

            // Skip if both are static
            bool aIsDynamic = false;
            bool bIsDynamic = false;
            if (registry.Has<RigidBodyComponent>(eA)) {
                aIsDynamic = (registry.Get<RigidBodyComponent>(eA).motionType == MotionType::Dynamic);
            }
            if (registry.Has<RigidBodyComponent>(eB)) {
                bIsDynamic = (registry.Get<RigidBodyComponent>(eB).motionType == MotionType::Dynamic);
            }
            if (!aIsDynamic && !bIsDynamic) continue;

            ContactPoint cp{};
            bool hit = false;

            // Narrowphase dispatch
            if (colA.type == ColliderType::Sphere && colB.type == ColliderType::Sphere) {
                hit = Collision::IntersectSphereSphere(posA, colA.sphereRadius, posB, colB.sphereRadius, cp);
            } else if (colA.type == ColliderType::Sphere && colB.type == ColliderType::Box) {
                hit = Collision::IntersectSphereBox(posA, colA.sphereRadius, posB, colB.boxHalfExtents, cp);
            } else if (colA.type == ColliderType::Box && colB.type == ColliderType::Sphere) {
                hit = Collision::IntersectSphereBox(posB, colB.sphereRadius, posA, colA.boxHalfExtents, cp);
                if (hit) cp.normal = cp.normal * -1.0f;
            } else if (colA.type == ColliderType::Box && colB.type == ColliderType::Box) {
                hit = Collision::IntersectBoxBox(posA, colA.boxHalfExtents, posB, colB.boxHalfExtents, cp);
            } else if (colA.type == ColliderType::Sphere && colB.type == ColliderType::Plane) {
                hit = Collision::IntersectSpherePlane(posA, colA.sphereRadius, colB.planeNormal, colB.planeDistance, cp);
            } else if (colA.type == ColliderType::Plane && colB.type == ColliderType::Sphere) {
                hit = Collision::IntersectSpherePlane(posB, colB.sphereRadius, colA.planeNormal, colA.planeDistance, cp);
                if (hit) cp.normal = cp.normal * -1.0f;
            } else if (colA.type == ColliderType::Box && colB.type == ColliderType::Plane) {
                hit = Collision::IntersectBoxPlane(posA, colA.boxHalfExtents, colB.planeNormal, colB.planeDistance, cp);
            } else if (colA.type == ColliderType::Plane && colB.type == ColliderType::Box) {
                hit = Collision::IntersectBoxPlane(posB, colB.boxHalfExtents, colA.planeNormal, colA.planeDistance, cp);
                if (hit) cp.normal = cp.normal * -1.0f;
            }

            if (hit) {
                m_activeContacts.push_back(ContactManifold{eA, eB, cp, true});
            }
        }
    }
}

void PhysicsScene::ResolveCollisions(World::Scene& worldScene, float dt) {
    (void)dt;
    auto& registry = worldScene.GetRegistry();

    for (const auto& manifold : m_activeContacts) {
        const World::EntityID eA = manifold.entityA;
        const World::EntityID eB = manifold.entityB;
        const auto& contact = manifold.contact;

        const bool hasRbA = registry.Has<RigidBodyComponent>(eA);
        const bool hasRbB = registry.Has<RigidBodyComponent>(eB);

        RigidBodyComponent* rbA = hasRbA ? &registry.Get<RigidBodyComponent>(eA) : nullptr;
        RigidBodyComponent* rbB = hasRbB ? &registry.Get<RigidBodyComponent>(eB) : nullptr;

        const bool aDynamic = rbA && (rbA->motionType == MotionType::Dynamic);
        const bool bDynamic = rbB && (rbB->motionType == MotionType::Dynamic);

        auto& tA = registry.Get<World::TransformComponent>(eA);
        auto& tB = registry.Get<World::TransformComponent>(eB);

        // 1. Positional Separation (Relaxation)
        const float invMassA = aDynamic ? rbA->invMass : 0.0f;
        const float invMassB = bDynamic ? rbB->invMass : 0.0f;
        const float totalInvMass = invMassA + invMassB;

        if (totalInvMass > 1e-6f) {
            const Core::Vec3 separation = contact.normal * (contact.penetrationDepth / totalInvMass);
            if (aDynamic) tA.position = tA.position - separation * invMassA;
            if (bDynamic) tB.position = tB.position + separation * invMassB;
        }

        // 2. Velocity Impulse Resolution
        Core::Vec3 velA = aDynamic ? rbA->linearVelocity : Core::Vec3{0.0f, 0.0f, 0.0f};
        Core::Vec3 velB = bDynamic ? rbB->linearVelocity : Core::Vec3{0.0f, 0.0f, 0.0f};

        const Core::Vec3 relativeVel = velB - velA;
        const float normalVelocity = relativeVel.Dot(contact.normal);

        // Moving apart already
        if (normalVelocity > 0.0f) continue;

        float restitution = 0.2f;
        if (registry.Has<ColliderComponent>(eA)) {
            restitution = std::max(restitution, registry.Get<ColliderComponent>(eA).restitution);
        }
        if (registry.Has<ColliderComponent>(eB)) {
            restitution = std::max(restitution, registry.Get<ColliderComponent>(eB).restitution);
        }

        const float impulseMag = (-(1.0f + restitution) * normalVelocity) / totalInvMass;
        const Core::Vec3 impulse = contact.normal * impulseMag;

        if (aDynamic) rbA->linearVelocity = rbA->linearVelocity - impulse * invMassA;
        if (bDynamic) rbB->linearVelocity = rbB->linearVelocity + impulse * invMassB;
    }
}

bool PhysicsScene::Raycast(const Ray& ray, RaycastHit& outHit, World::Scene& worldScene) const {
    auto& registry = worldScene.GetRegistry();
    if (!registry.HasPool<ColliderComponent>()) return false;

    auto& colPool = registry.GetPool<ColliderComponent>();
    const auto& colliders = colPool.GetData();
    const auto& entities = colPool.GetEntities();

    bool foundHit = false;
    float closestDist = ray.maxDistance;
    RaycastHit bestHit{};

    for (size_t i = 0; i < colliders.size(); ++i) {
        const auto& col = colliders[i];
        const World::EntityID e = entities[i];
        if (!registry.Has<World::TransformComponent>(e)) continue;

        const auto& t = registry.Get<World::TransformComponent>(e);
        const Core::Vec3 center = t.position + col.offset;

        RaycastHit currentHit{};
        bool hit = false;

        if (col.type == ColliderType::Sphere) {
            hit = Collision::RaycastSphere(ray, center, col.sphereRadius, currentHit);
        } else if (col.type == ColliderType::Box) {
            Core::AABB aabb{center - col.boxHalfExtents, center + col.boxHalfExtents};
            hit = Collision::RaycastAABB(ray, aabb, currentHit);
        }

        if (hit && currentHit.distance < closestDist) {
            closestDist = currentHit.distance;
            bestHit = currentHit;
            bestHit.entity = e;
            foundHit = true;
        }
    }

    if (foundHit) {
        outHit = bestHit;
        return true;
    }
    return false;
}

} // namespace Engine::Physics
