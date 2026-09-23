#pragma once

#include "engine/animation/Skeleton.h"
#include "engine/animation/AnimationClip.h"
#include <memory>
#include <vector>

namespace Engine::Animation {

class Animator {
public:
    explicit Animator(const Skeleton* skeleton);
    ~Animator() = default;

    void PlayClip(const AnimationClip* clip, bool loop = true);
    void Update(float deltaTime);

    [[nodiscard]] const std::vector<Core::Mat4x4>& GetSkinningPalette() const noexcept {
        return m_skinningPalette;
    }

    [[nodiscard]] const std::vector<Core::Mat4x4>& GetGlobalPose() const noexcept {
        return m_globalPose;
    }

    [[nodiscard]] float GetCurrentTime() const noexcept { return m_currentTime; }
    [[nodiscard]] bool IsPlaying() const noexcept { return m_isPlaying; }

    void SetPlaybackSpeed(float speed) noexcept { m_playbackSpeed = speed; }

private:
    const Skeleton* m_skeleton{nullptr};
    const AnimationClip* m_currentClip{nullptr};

    float m_currentTime{0.0f};
    float m_playbackSpeed{1.0f};
    bool m_loop{true};
    bool m_isPlaying{false};

    std::vector<JointTransform> m_localPose;
    std::vector<Core::Mat4x4> m_globalPose;
    std::vector<Core::Mat4x4> m_skinningPalette;
};

} // namespace Engine::Animation
