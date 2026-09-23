#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include <vector>

namespace Engine::Animation {

struct alignas(16) SkinnedVertex {
    Core::Vec3 position;
    float pad0{0.0f};
    Core::Vec3 normal;
    float pad1{0.0f};
    Core::Vec4 tangent;
    float uv[2]{0.0f, 0.0f};
    float pad2[2]{0.0f, 0.0f};
    Core::u32 boneIndices[4]{0, 0, 0, 0};
    float boneWeights[4]{1.0f, 0.0f, 0.0f, 0.0f};
};

class SkinningPass {
public:
    SkinningPass() = default;
    ~SkinningPass() = default;

    static void DeformVertices(
        const std::vector<SkinnedVertex>& inVertices,
        const std::vector<Core::Mat4x4>& bonePalette,
        std::vector<Core::Vec3>& outPositions,
        std::vector<Core::Vec3>& outNormals
    );
};

} // namespace Engine::Animation
