#pragma once

#include "engine/physics/PhysicsTypes.h"
#include "engine/core/MathTypes.h"

namespace Engine::Physics {

class Collision {
public:
    static bool IntersectSphereSphere(
        const Core::Vec3& posA, float radiusA,
        const Core::Vec3& posB, float radiusB,
        ContactPoint& outContact
    ) noexcept;

    static bool IntersectSphereBox(
        const Core::Vec3& spherePos, float radius,
        const Core::Vec3& boxPos, const Core::Vec3& boxHalfExtents,
        ContactPoint& outContact
    ) noexcept;

    static bool IntersectBoxBox(
        const Core::Vec3& posA, const Core::Vec3& halfA,
        const Core::Vec3& posB, const Core::Vec3& halfB,
        ContactPoint& outContact
    ) noexcept;

    static bool IntersectSpherePlane(
        const Core::Vec3& spherePos, float radius,
        const Core::Vec3& planeNormal, float planeDist,
        ContactPoint& outContact
    ) noexcept;

    static bool IntersectBoxPlane(
        const Core::Vec3& boxPos, const Core::Vec3& boxHalfExtents,
        const Core::Vec3& planeNormal, float planeDist,
        ContactPoint& outContact
    ) noexcept;

    static bool RaycastSphere(
        const Ray& ray,
        const Core::Vec3& sphereCenter,
        float radius,
        RaycastHit& outHit
    ) noexcept;

    static bool RaycastAABB(
        const Ray& ray,
        const Core::AABB& aabb,
        RaycastHit& outHit
    ) noexcept;
};

} // namespace Engine::Physics
