#include "engine/renderer/CascadedShadowMaps.h"
#include <cmath>
#include <algorithm>

namespace Engine::Renderer {

CascadedShadowMaps::CascadedShadowMaps() = default;

void CascadedShadowMaps::Update(
    const Camera& camera,
    const Core::Vec3& lightDirection,
    float maxShadowDistance,
    float splitLambda
) {
    const float nearZ = camera.GetNearZ();
    const float farZ = std::min(camera.GetFarZ(), maxShadowDistance);
    const Core::Vec3 normalizedLight = lightDirection.Normalized();

    float prevSplit = nearZ;

    for (size_t i = 0; i < CASCADE_COUNT; ++i) {
        const float p = static_cast<float>(i + 1) / static_cast<float>(CASCADE_COUNT);
        const float logSplit = nearZ * std::pow(farZ / nearZ, p);
        const float uniSplit = nearZ + (farZ - nearZ) * p;
        const float splitDist = splitLambda * logSplit + (1.0f - splitLambda) * uniSplit;

        m_cascades[i].splitDistance = splitDist;

        // Cascade sub-frustum center & radius approximation
        const float cascadeCenterDist = (prevSplit + splitDist) * 0.5f;
        const float cascadeRadius = (splitDist - prevSplit) * 0.6f + 2.0f;

        const Core::Vec3 camPos = camera.GetPosition();
        const Core::Vec3 camForward = (camera.GetTarget() - camPos).Normalized();
        const Core::Vec3 center = camPos + camForward * cascadeCenterDist;

        // Position directional light camera looking at center
        const Core::Vec3 lightEye = center - normalizedLight * (cascadeRadius * 2.0f);
        const Core::Mat4x4 lightView = Core::Mat4x4::LookAtLH(lightEye, center, {0.0f, 1.0f, 0.0f});

        // Texel snapping to eliminate shimmering
        const float worldUnitsPerTexel = (cascadeRadius * 2.0f) / static_cast<float>(m_resolution);
        const float snappedRadius = std::ceil(cascadeRadius / worldUnitsPerTexel) * worldUnitsPerTexel;

        // Orthographic projection
        Core::Mat4x4 lightProj{};
        lightProj.m[0]  = 1.0f / snappedRadius;
        lightProj.m[5]  = 1.0f / snappedRadius;
        lightProj.m[10] = 1.0f / (snappedRadius * 4.0f); // Depth range [0, 1]
        lightProj.m[15] = 1.0f;

        m_cascades[i].lightViewProj = lightView * lightProj;
        prevSplit = splitDist;
    }
}

} // namespace Engine::Renderer
