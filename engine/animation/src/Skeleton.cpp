#include "engine/animation/Skeleton.h"
#include "engine/core/Assert.h"

namespace Engine::Animation {

Core::i16 Skeleton::AddJoint(std::string name, Core::i16 parentIndex, const Core::Mat4x4& inverseBindPose) {
    // Defensive check: parent must precede child for linear topological traversal
    ENGINE_ASSERT(parentIndex < static_cast<Core::i16>(m_joints.size()), "Parent joint must be added before child");
    
    Joint joint;
    joint.name = std::move(name);
    joint.parentIndex = parentIndex;
    joint.inverseBindPose = inverseBindPose;

    const Core::i16 newIdx = static_cast<Core::i16>(m_joints.size());
    m_joints.push_back(std::move(joint));
    return newIdx;
}

Core::i16 Skeleton::FindJointIndex(const std::string& name) const noexcept {
    for (size_t i = 0; i < m_joints.size(); ++i) {
        if (m_joints[i].name == name) {
            return static_cast<Core::i16>(i);
        }
    }
    return -1;
}

} // namespace Engine::Animation
