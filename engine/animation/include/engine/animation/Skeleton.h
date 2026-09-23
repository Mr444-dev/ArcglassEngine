#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include <string>
#include <vector>

namespace Engine::Animation {

struct Joint {
    std::string name;
    Core::i16 parentIndex{-1};
    Core::Mat4x4 inverseBindPose{Core::Mat4x4::Identity()};
};

class Skeleton {
public:
    Skeleton() = default;
    ~Skeleton() = default;

    Core::i16 AddJoint(std::string name, Core::i16 parentIndex, const Core::Mat4x4& inverseBindPose);

    [[nodiscard]] size_t GetJointCount() const noexcept { return m_joints.size(); }
    [[nodiscard]] const Joint& GetJoint(size_t index) const noexcept { return m_joints[index]; }
    [[nodiscard]] const std::vector<Joint>& GetJoints() const noexcept { return m_joints; }

    [[nodiscard]] Core::i16 FindJointIndex(const std::string& name) const noexcept;

private:
    std::vector<Joint> m_joints;
};

} // namespace Engine::Animation
