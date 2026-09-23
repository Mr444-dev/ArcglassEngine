#include "engine/physics/Collision.h"
#include <cmath>
#include <algorithm>

namespace Engine::Physics {

bool Collision::IntersectSphereSphere(
    const Core::Vec3& posA, float radiusA,
    const Core::Vec3& posB, float radiusB,
    ContactPoint& outContact
) noexcept {
    const Core::Vec3 delta = posB - posA;
    const float dist = delta.Length();
    const float totalRadius = radiusA + radiusB;

    if (dist >= totalRadius) {
        return false;
    }

    outContact.normal = (dist > 1e-6f) ? (delta * (1.0f / dist)) : Core::Vec3{0.0f, 1.0f, 0.0f};
    outContact.penetrationDepth = totalRadius - dist;
    outContact.position = posA + outContact.normal * (radiusA - outContact.penetrationDepth * 0.5f);
    return true;
}

bool Collision::IntersectSphereBox(
    const Core::Vec3& spherePos, float radius,
    const Core::Vec3& boxPos, const Core::Vec3& boxHalfExtents,
    ContactPoint& outContact
) noexcept {
    // Find closest point on box to sphere
    const Core::Vec3 boxMin = boxPos - boxHalfExtents;
    const Core::Vec3 boxMax = boxPos + boxHalfExtents;

    const Core::Vec3 closest{
        std::clamp(spherePos.x, boxMin.x, boxMax.x),
        std::clamp(spherePos.y, boxMin.y, boxMax.y),
        std::clamp(spherePos.z, boxMin.z, boxMax.z)
    };

    const Core::Vec3 diff = spherePos - closest;
    const float distSq = diff.Dot(diff);

    if (distSq >= radius * radius) {
        return false;
    }

    const float dist = std::sqrt(distSq);
    if (dist > 1e-6f) {
        outContact.normal = diff * (1.0f / dist);
        outContact.penetrationDepth = radius - dist;
        outContact.position = closest;
    } else {
        // Sphere center inside box: find closest face
        const float dx = boxHalfExtents.x - std::abs(spherePos.x - boxPos.x);
        const float dy = boxHalfExtents.y - std::abs(spherePos.y - boxPos.y);
        const float dz = boxHalfExtents.z - std::abs(spherePos.z - boxPos.z);

        if (dx < dy && dx < dz) {
            outContact.normal = {(spherePos.x > boxPos.x) ? 1.0f : -1.0f, 0.0f, 0.0f};
            outContact.penetrationDepth = dx + radius;
        } else if (dy < dz) {
            outContact.normal = {0.0f, (spherePos.y > boxPos.y) ? 1.0f : -1.0f, 0.0f};
            outContact.penetrationDepth = dy + radius;
        } else {
            outContact.normal = {0.0f, 0.0f, (spherePos.z > boxPos.z) ? 1.0f : -1.0f};
            outContact.penetrationDepth = dz + radius;
        }
        outContact.position = spherePos;
    }

    return true;
}

bool Collision::IntersectBoxBox(
    const Core::Vec3& posA, const Core::Vec3& halfA,
    const Core::Vec3& posB, const Core::Vec3& halfB,
    ContactPoint& outContact
) noexcept {
    const Core::Vec3 delta = posB - posA;

    const float overlapX = (halfA.x + halfB.x) - std::abs(delta.x);
    if (overlapX <= 0.0f) return false;

    const float overlapY = (halfA.y + halfB.y) - std::abs(delta.y);
    if (overlapY <= 0.0f) return false;

    const float overlapZ = (halfA.z + halfB.z) - std::abs(delta.z);
    if (overlapZ <= 0.0f) return false;

    // Minimum penetration axis (SAT)
    if (overlapX < overlapY && overlapX < overlapZ) {
        outContact.normal = {(delta.x > 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f};
        outContact.penetrationDepth = overlapX;
        outContact.position = posA + Core::Vec3{outContact.normal.x * halfA.x, 0.0f, 0.0f};
    } else if (overlapY < overlapZ) {
        outContact.normal = {0.0f, (delta.y > 0.0f) ? 1.0f : -1.0f, 0.0f};
        outContact.penetrationDepth = overlapY;
        outContact.position = posA + Core::Vec3{0.0f, outContact.normal.y * halfA.y, 0.0f};
    } else {
        outContact.normal = {0.0f, 0.0f, (delta.z > 0.0f) ? 1.0f : -1.0f};
        outContact.penetrationDepth = overlapZ;
        outContact.position = posA + Core::Vec3{0.0f, 0.0f, outContact.normal.z * halfA.z};
    }

    return true;
}

bool Collision::IntersectSpherePlane(
    const Core::Vec3& spherePos, float radius,
    const Core::Vec3& planeNormal, float planeDist,
    ContactPoint& outContact
) noexcept {
    const float dist = planeNormal.Dot(spherePos) + planeDist;
    if (dist >= radius) {
        return false;
    }

    outContact.normal = planeNormal;
    outContact.penetrationDepth = radius - dist;
    outContact.position = spherePos - planeNormal * dist;
    return true;
}

bool Collision::IntersectBoxPlane(
    const Core::Vec3& boxPos, const Core::Vec3& boxHalfExtents,
    const Core::Vec3& planeNormal, float planeDist,
    ContactPoint& outContact
) noexcept {
    const float dist = planeNormal.Dot(boxPos) + planeDist;
    const float r = boxHalfExtents.x * std::abs(planeNormal.x) +
                    boxHalfExtents.y * std::abs(planeNormal.y) +
                    boxHalfExtents.z * std::abs(planeNormal.z);

    if (dist >= r) {
        return false;
    }

    outContact.normal = planeNormal;
    outContact.penetrationDepth = r - dist;
    outContact.position = boxPos - planeNormal * dist;
    return true;
}

bool Collision::RaycastSphere(
    const Ray& ray,
    const Core::Vec3& sphereCenter,
    float radius,
    RaycastHit& outHit
) noexcept {
    const Core::Vec3 m = ray.origin - sphereCenter;
    const float b = m.Dot(ray.direction);
    const float c = m.Dot(m) - radius * radius;

    if (c > 0.0f && b > 0.0f) return false;

    const float discr = b * b - c;
    if (discr < 0.0f) return false;

    float t = -b - std::sqrt(discr);
    if (t < 0.0f) t = 0.0f;
    if (t > ray.maxDistance) return false;

    outHit.hit = true;
    outHit.distance = t;
    outHit.point = ray.origin + ray.direction * t;
    outHit.normal = (outHit.point - sphereCenter).Normalized();
    return true;
}

bool Collision::RaycastAABB(
    const Ray& ray,
    const Core::AABB& aabb,
    RaycastHit& outHit
) noexcept {
    float tmin = 0.0f;
    float tmax = ray.maxDistance;
    Core::Vec3 hitNormal{0.0f, 0.0f, 0.0f};

    for (int i = 0; i < 3; ++i) {
        const float origin = (i == 0) ? ray.origin.x : ((i == 1) ? ray.origin.y : ray.origin.z);
        const float dir = (i == 0) ? ray.direction.x : ((i == 1) ? ray.direction.y : ray.direction.z);
        const float minVal = (i == 0) ? aabb.min.x : ((i == 1) ? aabb.min.y : aabb.min.z);
        const float maxVal = (i == 0) ? aabb.max.x : ((i == 1) ? aabb.max.y : aabb.max.z);

        if (std::abs(dir) < 1e-6f) {
            if (origin < minVal || origin > maxVal) return false;
        } else {
            const float ood = 1.0f / dir;
            float t1 = (minVal - origin) * ood;
            float t2 = (maxVal - origin) * ood;
            Core::Vec3 n = (i == 0) ? Core::Vec3{-1.0f, 0.0f, 0.0f} : ((i == 1) ? Core::Vec3{0.0f, -1.0f, 0.0f} : Core::Vec3{0.0f, 0.0f, -1.0f});

            if (t1 > t2) {
                std::swap(t1, t2);
                n = n * -1.0f;
            }

            if (t1 > tmin) {
                tmin = t1;
                hitNormal = n;
            }
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return false;
        }
    }

    outHit.hit = true;
    outHit.distance = tmin;
    outHit.point = ray.origin + ray.direction * tmin;
    outHit.normal = hitNormal;
    return true;
}

} // namespace Engine::Physics
