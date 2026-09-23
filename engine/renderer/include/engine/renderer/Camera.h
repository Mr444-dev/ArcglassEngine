#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include <array>
#include <string>

namespace Engine::Renderer {

using Core::Vec3;
using Core::Mat4x4;
using Core::AABB;

struct Plane {
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float distance{0.0f};

    [[nodiscard]] float DistanceToPoint(const Vec3& point) const noexcept {
        return normal.Dot(point) + distance;
    }
};

class Frustum {
public:
    enum PlaneIndex : size_t {
        Left = 0,
        Right = 1,
        Bottom = 2,
        Top = 3,
        Near = 4,
        Far = 5,
        Count = 6
    };

    void UpdateFromMatrix(const Mat4x4& viewProjection) noexcept;
    [[nodiscard]] bool IntersectsAABB(const AABB& aabb) const noexcept;

    [[nodiscard]] const Plane& GetPlane(PlaneIndex index) const noexcept { return m_planes[index]; }

private:
    std::array<Plane, 6> m_planes;
};

class Camera {
public:
    Camera();
    ~Camera() = default;

    void SetPosition(const Vec3& pos) noexcept { m_position = pos; m_dirty = true; }
    void SetTarget(const Vec3& target) noexcept { m_target = target; m_dirty = true; }
    void SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ) noexcept;

    void UpdateMatrices();

    [[nodiscard]] const Vec3& GetPosition() const noexcept { return m_position; }
    [[nodiscard]] const Vec3& GetTarget() const noexcept { return m_target; }
    [[nodiscard]] const Mat4x4& GetViewMatrix() const noexcept { return m_view; }
    [[nodiscard]] const Mat4x4& GetProjectionMatrix() const noexcept { return m_proj; }
    [[nodiscard]] const Mat4x4& GetViewProjectionMatrix() const noexcept { return m_viewProj; }
    [[nodiscard]] const Frustum& GetFrustum() const noexcept { return m_frustum; }

    [[nodiscard]] float GetNearZ() const noexcept { return m_nearZ; }
    [[nodiscard]] float GetFarZ() const noexcept { return m_farZ; }
    [[nodiscard]] float GetFovY() const noexcept { return m_fovY; }
    [[nodiscard]] float GetAspectRatio() const noexcept { return m_aspect; }

    [[nodiscard]] std::string GetDebugString() const;

private:
    Vec3 m_position{0.0f, 2.0f, -5.0f};
    Vec3 m_target{0.0f, 0.0f, 0.0f};
    Vec3 m_up{0.0f, 1.0f, 0.0f};

    float m_fovY{0.785398f};
    float m_aspect{16.0f / 9.0f};
    float m_nearZ{0.1f};
    float m_farZ{1000.0f};

    Mat4x4 m_view{};
    Mat4x4 m_proj{};
    Mat4x4 m_viewProj{};
    Frustum m_frustum{};

    bool m_dirty{true};
};

} // namespace Engine::Renderer
