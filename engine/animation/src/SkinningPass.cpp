#include "engine/animation/SkinningPass.h"
#include <cmath>

namespace Engine::Animation {

void SkinningPass::DeformVertices(
    const std::vector<SkinnedVertex>& inVertices,
    const std::vector<Core::Mat4x4>& bonePalette,
    std::vector<Core::Vec3>& outPositions,
    std::vector<Core::Vec3>& outNormals
) {
    outPositions.resize(inVertices.size());
    outNormals.resize(inVertices.size());

    for (size_t i = 0; i < inVertices.size(); ++i) {
        const auto& vin = inVertices[i];

        const float totalWeight = vin.boneWeights[0] + vin.boneWeights[1] + vin.boneWeights[2] + vin.boneWeights[3];
        const float w0 = (totalWeight > 1e-4f) ? (vin.boneWeights[0] / totalWeight) : 1.0f;
        const float w1 = (totalWeight > 1e-4f) ? (vin.boneWeights[1] / totalWeight) : 0.0f;
        const float w2 = (totalWeight > 1e-4f) ? (vin.boneWeights[2] / totalWeight) : 0.0f;
        const float w3 = (totalWeight > 1e-4f) ? (vin.boneWeights[3] / totalWeight) : 0.0f;

        Core::Mat4x4 skinMatrix{};
        for (int m = 0; m < 16; ++m) {
            skinMatrix.m[m] =
                bonePalette[vin.boneIndices[0]].m[m] * w0 +
                bonePalette[vin.boneIndices[1]].m[m] * w1 +
                bonePalette[vin.boneIndices[2]].m[m] * w2 +
                bonePalette[vin.boneIndices[3]].m[m] * w3;
        }

        // Apply matrix transform to position (w = 1.0)
        const Core::Vec3 p = vin.position;
        outPositions[i] = {
            p.x * skinMatrix.m[0] + p.y * skinMatrix.m[4] + p.z * skinMatrix.m[8]  + skinMatrix.m[12],
            p.x * skinMatrix.m[1] + p.y * skinMatrix.m[5] + p.z * skinMatrix.m[9]  + skinMatrix.m[13],
            p.x * skinMatrix.m[2] + p.y * skinMatrix.m[6] + p.z * skinMatrix.m[10] + skinMatrix.m[14]
        };

        // Apply matrix transform to normal (w = 0.0)
        const Core::Vec3 n = vin.normal;
        Core::Vec3 deformedNormal{
            n.x * skinMatrix.m[0] + n.y * skinMatrix.m[4] + n.z * skinMatrix.m[8],
            n.x * skinMatrix.m[1] + n.y * skinMatrix.m[5] + n.z * skinMatrix.m[9],
            n.x * skinMatrix.m[2] + n.y * skinMatrix.m[6] + n.z * skinMatrix.m[10]
        };
        outNormals[i] = deformedNormal.Normalized();
    }
}

} // namespace Engine::Animation
