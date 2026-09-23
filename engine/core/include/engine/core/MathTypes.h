#pragma once

#include "engine/core/BaseTypes.h"
#include <cmath>
#include <string>

namespace Engine::Core {

struct Vec2 {
    float x{0.0f}, y{0.0f};
};

struct Vec3 {
    float x{0.0f}, y{0.0f}, z{0.0f};

    [[nodiscard]] Vec3 operator+(const Vec3& o) const noexcept { return {x + o.x, y + o.y, z + o.z}; }
    [[nodiscard]] Vec3 operator-(const Vec3& o) const noexcept { return {x - o.x, y - o.y, z - o.z}; }
    [[nodiscard]] Vec3 operator*(float s) const noexcept { return {x * s, y * s, z * s}; }
    [[nodiscard]] float Dot(const Vec3& o) const noexcept { return x * o.x + y * o.y + z * o.z; }
    [[nodiscard]] Vec3 Cross(const Vec3& o) const noexcept {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }
    [[nodiscard]] float Length() const noexcept {
        return std::sqrt(x * x + y * y + z * z);
    }
    [[nodiscard]] Vec3 Normalized() const noexcept {
        const float len = Length();
        if (len > 1e-6f) {
            const float inv = 1.0f / len;
            return {x * inv, y * inv, z * inv};
        }
        return {0.0f, 0.0f, 0.0f};
    }
};

struct Vec4 {
    float x{0.0f}, y{0.0f}, z{0.0f}, w{0.0f};
};

struct Mat4x4;

struct Quat {
    float x{0.0f}, y{0.0f}, z{0.0f}, w{1.0f};

    static Quat Identity() noexcept { return {0.0f, 0.0f, 0.0f, 1.0f}; }

    static Quat FromEuler(float pitch, float yaw, float roll) noexcept {
        const float p = pitch * 0.5f;
        const float y = yaw * 0.5f;
        const float r = roll * 0.5f;

        const float sinP = std::sin(p);
        const float cosP = std::cos(p);
        const float sinY = std::sin(y);
        const float cosY = std::cos(y);
        const float sinR = std::sin(r);
        const float cosR = std::cos(r);

        return {
            sinP * cosY * cosR - cosP * sinY * sinR,
            cosP * sinY * cosR + sinP * cosY * sinR,
            cosP * cosY * sinR - sinP * sinY * cosR,
            cosP * cosY * cosR + sinP * sinY * sinR
        };
    }

    [[nodiscard]] float Dot(const Quat& o) const noexcept {
        return x * o.x + y * o.y + z * o.z + w * o.w;
    }

    [[nodiscard]] Quat Slerp(const Quat& end, float t) const noexcept {
        float cosHalfTheta = Dot(end);
        Quat target = end;
        if (cosHalfTheta < 0.0f) {
            target = {-end.x, -end.y, -end.z, -end.w};
            cosHalfTheta = -cosHalfTheta;
        }

        if (std::abs(cosHalfTheta) >= 0.999f) {
            return {
                x * (1.0f - t) + target.x * t,
                y * (1.0f - t) + target.y * t,
                z * (1.0f - t) + target.z * t,
                w * (1.0f - t) + target.w * t
            };
        }

        const float halfTheta = std::acos(cosHalfTheta);
        const float sinHalfTheta = std::sqrt(1.0f - cosHalfTheta * cosHalfTheta);

        const float ratioA = std::sin((1.0f - t) * halfTheta) / sinHalfTheta;
        const float ratioB = std::sin(t * halfTheta) / sinHalfTheta;

        return {
            x * ratioA + target.x * ratioB,
            y * ratioA + target.y * ratioB,
            z * ratioA + target.z * ratioB,
            w * ratioA + target.w * ratioB
        };
    }

    [[nodiscard]] Mat4x4 ToMatrix() const noexcept;
};

struct Mat4x4 {
    float m[16]{};

    static Mat4x4 Identity() noexcept {
        Mat4x4 res{};
        res.m[0]  = 1.0f; res.m[5]  = 1.0f;
        res.m[10] = 1.0f; res.m[15] = 1.0f;
        return res;
    }

    static Mat4x4 Translation(const Vec3& t) noexcept {
        Mat4x4 res = Identity();
        res.m[12] = t.x;
        res.m[13] = t.y;
        res.m[14] = t.z;
        return res;
    }

    static Mat4x4 Scaling(const Vec3& s) noexcept {
        Mat4x4 res{};
        res.m[0]  = s.x;
        res.m[5]  = s.y;
        res.m[10] = s.z;
        res.m[15] = 1.0f;
        return res;
    }

    static Mat4x4 Scale(const Vec3& s) noexcept { return Scaling(s); }

    static Mat4x4 RotationEuler(float pitch, float yaw, float roll) noexcept {
        const float cp = std::cos(pitch);
        const float sp = std::sin(pitch);
        const float cy = std::cos(yaw);
        const float sy = std::sin(yaw);
        const float cr = std::cos(roll);
        const float sr = std::sin(roll);

        Mat4x4 res = Identity();
        res.m[0] = cy * cr + sy * sp * sr;
        res.m[1] = sr * cp;
        res.m[2] = -sy * cr + cy * sp * sr;

        res.m[4] = -cy * sr + sy * sp * cr;
        res.m[5] = cr * cp;
        res.m[6] = sr * sy + cy * sp * cr;

        res.m[8] = sy * cp;
        res.m[9] = -sp;
        res.m[10] = cy * cp;
        return res;
    }

    static Mat4x4 Transform(const Vec3& pos, const Vec3& eulerRot, const Vec3& scale) noexcept {
        // DirectX row-vector convention: S * R * T (scale, rotate, then translate)
        Mat4x4 s = Scaling(scale);
        Mat4x4 r = RotationEuler(eulerRot.x, eulerRot.y, eulerRot.z);
        Mat4x4 t = Translation(pos);
        return s * r * t;
    }

    static Mat4x4 LookAtLH(const Vec3& eye, const Vec3& target, const Vec3& up) noexcept {
        Vec3 zAxis = (target - eye).Normalized();
        Vec3 xAxis = up.Cross(zAxis).Normalized();
        Vec3 yAxis = zAxis.Cross(xAxis);

        Mat4x4 res = Identity();
        res.m[0] = xAxis.x; res.m[4] = xAxis.y; res.m[8]  = xAxis.z; res.m[12] = -xAxis.Dot(eye);
        res.m[1] = yAxis.x; res.m[5] = yAxis.y; res.m[9]  = yAxis.z; res.m[13] = -yAxis.Dot(eye);
        res.m[2] = zAxis.x; res.m[6] = zAxis.y; res.m[10] = zAxis.z; res.m[14] = -zAxis.Dot(eye);
        return res;
    }

    static Mat4x4 PerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ) noexcept {
        const float sinFov = std::sin(0.5f * fovY);
        const float cosFov = std::cos(0.5f * fovY);
        const float height = cosFov / sinFov;
        const float width  = height / aspect;
        const float fRange = farZ / (farZ - nearZ);

        Mat4x4 res{};
        res.m[0]  = width;
        res.m[5]  = height;
        res.m[10] = fRange;
        res.m[11] = 1.0f;
        res.m[14] = -nearZ * fRange;
        return res;
    }

    [[nodiscard]] Mat4x4 operator*(const Mat4x4& o) const noexcept {
        Mat4x4 res{};
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += m[r + k * 4] * o.m[k + c * 4];
                }
                res.m[r + c * 4] = sum;
            }
        }
        return res;
    }
};

inline Mat4x4 Quat::ToMatrix() const noexcept {
    Mat4x4 res = Mat4x4::Identity();
    const float xx = x * x, yy = y * y, zz = z * z;
    const float xy = x * y, xz = x * z, yz = y * z;
    const float wx = w * x, wy = w * y, wz = w * z;

    res.m[0] = 1.0f - 2.0f * (yy + zz);
    res.m[1] = 2.0f * (xy + wz);
    res.m[2] = 2.0f * (xz - wy);

    res.m[4] = 2.0f * (xy - wz);
    res.m[5] = 1.0f - 2.0f * (xx + zz);
    res.m[6] = 2.0f * (yz + wx);

    res.m[8] = 2.0f * (xz + wy);
    res.m[9] = 2.0f * (yz - wx);
    res.m[10] = 1.0f - 2.0f * (xx + yy);

    return res;
}

struct AABB {
    Vec3 min{-1.0f, -1.0f, -1.0f};
    Vec3 max{ 1.0f,  1.0f,  1.0f};

    [[nodiscard]] Vec3 GetCenter() const noexcept { return (min + max) * 0.5f; }
    [[nodiscard]] Vec3 GetExtents() const noexcept { return (max - min) * 0.5f; }
    [[nodiscard]] float GetSurfaceArea() const noexcept {
        const Vec3 d = max - min;
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }

    void Encapsulate(const AABB& o) noexcept {
        min.x = std::min(min.x, o.min.x);
        min.y = std::min(min.y, o.min.y);
        min.z = std::min(min.z, o.min.z);
        max.x = std::max(max.x, o.max.x);
        max.y = std::max(max.y, o.max.y);
        max.z = std::max(max.z, o.max.z);
    }
};

} // namespace Engine::Core
