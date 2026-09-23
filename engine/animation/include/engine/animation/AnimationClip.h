#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include <string>
#include <vector>

namespace Engine::Animation {

struct TransformKey {
    float time{0.0f};
    Core::Vec3 translation{0.0f, 0.0f, 0.0f};
    Core::Quat rotation{Core::Quat::Identity()};
    Core::Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct JointTrack {
    Core::i16 jointIndex{-1};
    std::vector<TransformKey> keys;
};

struct JointTransform {
    Core::Vec3 translation{0.0f, 0.0f, 0.0f};
    Core::Quat rotation{Core::Quat::Identity()};
    Core::Vec3 scale{1.0f, 1.0f, 1.0f};

    [[nodiscard]] Core::Mat4x4 ToMatrix() const noexcept {
        const Core::Mat4x4 s = Core::Mat4x4::Scaling(scale);
        const Core::Mat4x4 r = rotation.ToMatrix();
        const Core::Mat4x4 t = Core::Mat4x4::Translation(translation);
        return s * r * t;
    }
};

class AnimationClip {
public:
    AnimationClip(std::string name, float duration)
        : m_name(std::move(name)), m_duration(duration) {}
    ~AnimationClip() = default;

    void AddTrack(JointTrack track);

    void Sample(float time, std::vector<JointTransform>& outLocalPose, bool loop = true) const;

    [[nodiscard]] const std::string& GetName() const noexcept { return m_name; }
    [[nodiscard]] float GetDuration() const noexcept { return m_duration; }
    [[nodiscard]] size_t GetTrackCount() const noexcept { return m_tracks.size(); }

private:
    std::string m_name;
    float m_duration{1.0f};
    std::vector<JointTrack> m_tracks;
};

} // namespace Engine::Animation
