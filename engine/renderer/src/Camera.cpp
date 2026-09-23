#include "engine/renderer/Camera.h"
#include "engine/core/Assert.h"
#include <cmath>
#include <format>

namespace Engine::Renderer {

// Gribb-Hartmann Frustum Plane Extraction
void Frustum::UpdateFromMatrix(const Mat4x4& vp) noexcept {
    // Left plane: col4 + col1
    m_planes[Left].normal.x = vp.m[3]  + vp.m[0];
    m_planes[Left].normal.y = vp.m[7]  + vp.m[4];
    m_planes[Left].normal.z = vp.m[11] + vp.m[8];
    m_planes[Left].distance = vp.m[15] + vp.m[12];

    // Right plane: col4 - col1
    m_planes[Right].normal.x = vp.m[3]  - vp.m[0];
    m_planes[Right].normal.y = vp.m[7]  - vp.m[4];
    m_planes[Right].normal.z = vp.m[11] - vp.m[8];
    m_planes[Right].distance = vp.m[15] - vp.m[12];

    // Bottom plane: col4 + col2
    m_planes[Bottom].normal.x = vp.m[3]  + vp.m[1];
    m_planes[Bottom].normal.y = vp.m[7]  + vp.m[5];
    m_planes[Bottom].normal.z = vp.m[11] + vp.m[9];
    m_planes[Bottom].distance = vp.m[15] + vp.m[13];

    // Top plane: col4 - col2
    m_planes[Top].normal.x = vp.m[3]  - vp.m[1];
    m_planes[Top].normal.y = vp.m[7]  - vp.m[5];
    m_planes[Top].normal.z = vp.m[11] - vp.m[9];
    m_planes[Top].distance = vp.m[15] - vp.m[13];

    // Near plane: col3
    m_planes[Near].normal.x = vp.m[2];
    m_planes[Near].normal.y = vp.m[6];
    m_planes[Near].normal.z = vp.m[10];
    m_planes[Near].distance = vp.m[14];

    // Far plane: col4 - col3
    m_planes[Far].normal.x = vp.m[3]  - vp.m[2];
    m_planes[Far].normal.y = vp.m[7]  - vp.m[6];
    m_planes[Far].normal.z = vp.m[11] - vp.m[10];
    m_planes[Far].distance = vp.m[15] - vp.m[14];

    // Normalize planes
    for (auto& plane : m_planes) {
        const float len = plane.normal.Length();
        if (len > 1e-6f) {
            const float invLen = 1.0f / len;
            plane.normal.x *= invLen;
            plane.normal.y *= invLen;
            plane.normal.z *= invLen;
            plane.distance *= invLen;
        }
    }
}

bool Frustum::IntersectsAABB(const AABB& aabb) const noexcept {
    for (const auto& plane : m_planes) {
        // Find p-vertex in the direction of the normal
        const Vec3 p{
            plane.normal.x >= 0.0f ? aabb.max.x : aabb.min.x,
            plane.normal.y >= 0.0f ? aabb.max.y : aabb.min.y,
            plane.normal.z >= 0.0f ? aabb.max.z : aabb.min.z
        };

        if (plane.DistanceToPoint(p) < 0.0f) {
            return false; // Point is behind the plane -> AABB is outside frustum
        }
    }
    return true; // Inside or intersecting all planes
}

Camera::Camera() {
    UpdateMatrices();
}

void Camera::SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ) noexcept {
    m_fovY = fovYRadians;
    m_aspect = aspectRatio;
    m_nearZ = nearZ;
    m_farZ = farZ;
    m_dirty = true;
}

void Camera::UpdateMatrices() {
    if (!m_dirty) return;

    m_view = Mat4x4::LookAtLH(m_position, m_target, m_up);
    m_proj = Mat4x4::PerspectiveFovLH(m_fovY, m_aspect, m_nearZ, m_farZ);
    m_viewProj = m_view * m_proj;
    m_frustum.UpdateFromMatrix(m_viewProj);

    m_dirty = false;
}

std::string Camera::GetDebugString() const {
    return std::format(
        "Camera [Pos: ({:.2f}, {:.2f}, {:.2f}) | Target: ({:.2f}, {:.2f}, {:.2f}) | FOV: {:.1f} deg | Aspect: {:.2f} | Near: {:.2f}, Far: {:.2f}]",
        m_position.x, m_position.y, m_position.z,
        m_target.x, m_target.y, m_target.z,
        m_fovY * (180.0f / 3.14159265f),
        m_aspect, m_nearZ, m_farZ
    );
}

} // namespace Engine::Renderer
