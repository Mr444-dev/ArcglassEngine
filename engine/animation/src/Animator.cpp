#include "engine/animation/Animator.h"

namespace Engine::Animation {

Animator::Animator(const Skeleton* skeleton)
    : m_skeleton(skeleton) {
    if (m_skeleton) {
        const size_t count = m_skeleton->GetJointCount();
        m_localPose.resize(count);
        m_globalPose.assign(count, Core::Mat4x4::Identity());
        m_skinningPalette.assign(count, Core::Mat4x4::Identity());
    }
}

void Animator::PlayClip(const AnimationClip* clip, bool loop) {
    m_currentClip = clip;
    m_loop = loop;
    m_currentTime = 0.0f;
    m_isPlaying = (m_currentClip != nullptr);
}

void Animator::Update(float deltaTime) {
    if (!m_isPlaying || !m_currentClip || !m_skeleton) return;

    m_currentTime += deltaTime * m_playbackSpeed;
    m_currentClip->Sample(m_currentTime, m_localPose, m_loop);

    const size_t jointCount = m_skeleton->GetJointCount();
    for (size_t i = 0; i < jointCount; ++i) {
        const Core::Mat4x4 localMat = m_localPose[i].ToMatrix();
        const Core::i16 parentIdx = m_skeleton->GetJoint(i).parentIndex;

        if (parentIdx < 0) {
            m_globalPose[i] = localMat;
        } else {
            // Non-recursive hierarchical concatenation: Child = Local * Parent
            m_globalPose[i] = localMat * m_globalPose[parentIdx];
        }

        // Final Skinning Matrix: InvBindPose * GlobalPose
        m_skinningPalette[i] = m_skeleton->GetJoint(i).inverseBindPose * m_globalPose[i];
    }
}

} // namespace Engine::Animation
