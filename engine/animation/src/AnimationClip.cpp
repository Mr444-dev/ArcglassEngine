#include "engine/animation/AnimationClip.h"
#include <cmath>
#include <algorithm>

namespace Engine::Animation {

void AnimationClip::AddTrack(JointTrack track) {
    m_tracks.push_back(std::move(track));
}

void AnimationClip::Sample(float time, std::vector<JointTransform>& outLocalPose, bool loop) const {
    if (m_duration <= 0.0f || m_tracks.empty()) return;

    float evalTime = time;
    if (loop) {
        evalTime = std::fmod(time, m_duration);
        if (evalTime < 0.0f) evalTime += m_duration;
    } else {
        evalTime = std::clamp(time, 0.0f, m_duration);
    }

    for (const auto& track : m_tracks) {
        if (track.jointIndex < 0 || static_cast<size_t>(track.jointIndex) >= outLocalPose.size()) {
            continue;
        }

        if (track.keys.empty()) continue;

        if (track.keys.size() == 1 || evalTime <= track.keys.front().time) {
            outLocalPose[track.jointIndex].translation = track.keys.front().translation;
            outLocalPose[track.jointIndex].rotation    = track.keys.front().rotation;
            outLocalPose[track.jointIndex].scale       = track.keys.front().scale;
            continue;
        }

        if (evalTime >= track.keys.back().time) {
            outLocalPose[track.jointIndex].translation = track.keys.back().translation;
            outLocalPose[track.jointIndex].rotation    = track.keys.back().rotation;
            outLocalPose[track.jointIndex].scale       = track.keys.back().scale;
            continue;
        }

        // Find surrounding keyframes
        size_t k0 = 0;
        for (size_t i = 0; i < track.keys.size() - 1; ++i) {
            if (evalTime >= track.keys[i].time && evalTime <= track.keys[i + 1].time) {
                k0 = i;
                break;
            }
        }
        const size_t k1 = k0 + 1;

        const float dt = track.keys[k1].time - track.keys[k0].time;
        const float alpha = (dt > 1e-6f) ? ((evalTime - track.keys[k0].time) / dt) : 0.0f;

        // Translation LERP
        outLocalPose[track.jointIndex].translation =
            track.keys[k0].translation * (1.0f - alpha) + track.keys[k1].translation * alpha;

        // Rotation SLERP
        outLocalPose[track.jointIndex].rotation =
            track.keys[k0].rotation.Slerp(track.keys[k1].rotation, alpha);

        // Scale LERP
        outLocalPose[track.jointIndex].scale =
            track.keys[k0].scale * (1.0f - alpha) + track.keys[k1].scale * alpha;
    }
}

} // namespace Engine::Animation
