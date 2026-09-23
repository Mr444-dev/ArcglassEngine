#include "engine/renderer/VoxelGlobalIllumination.h"
#include <cmath>
#include <algorithm>

namespace Engine::Renderer {

std::array<VoxelCone, 6> VoxelGlobalIllumination::GenerateDiffuseCones(const Core::Vec3& normal) noexcept {
    std::array<VoxelCone, 6> cones;

    const Core::Vec3 N = normal.Normalized();

    // 1. Center cone along surface normal
    cones[0] = {N, 0.577f, 0.25f};

    // Construct orthogonal tangent frame (T, B)
    Core::Vec3 up = (std::abs(N.y) < 0.999f) ? Core::Vec3{0.0f, 1.0f, 0.0f} : Core::Vec3{1.0f, 0.0f, 0.0f};
    Core::Vec3 T = up.Cross(N).Normalized();
    Core::Vec3 B = N.Cross(T).Normalized();

    constexpr float PI = 3.14159265358979323846f;
    constexpr float cos60 = 0.5f;
    constexpr float sin60 = 0.86602540378f;

    // 2. Five side cones at 60 degree elevation angle
    for (int i = 0; i < 5; ++i) {
        const float phi = (2.0f * PI * static_cast<float>(i)) / 5.0f;
        const float cosPhi = std::cos(phi);
        const float sinPhi = std::sin(phi);

        Core::Vec3 sideDir = (N * cos60) + (T * (sin60 * cosPhi)) + (B * (sin60 * sinPhi));
        cones[i + 1] = {sideDir.Normalized(), 0.577f, 0.15f};
    }

    return cones;
}

Core::Vec3 VoxelGlobalIllumination::WorldToVoxelUVW(
    const Core::Vec3& worldPos,
    const Core::Vec3& volumeCenter,
    float volumeExtent
) noexcept {
    if (volumeExtent < 1e-4f) return {0.5f, 0.5f, 0.5f};

    const Core::Vec3 halfExtent = {volumeExtent * 0.5f, volumeExtent * 0.5f, volumeExtent * 0.5f};
    const Core::Vec3 minCorner = volumeCenter - halfExtent;

    const Core::Vec3 offset = worldPos - minCorner;
    const float invExtent = 1.0f / volumeExtent;

    return {
        std::clamp(offset.x * invExtent, 0.0f, 1.0f),
        std::clamp(offset.y * invExtent, 0.0f, 1.0f),
        std::clamp(offset.z * invExtent, 0.0f, 1.0f)
    };
}

void VoxelGlobalIllumination::AddToGraph(
    RenderGraph& rg,
    RGTextureHandle gBufferNormal,
    RGTextureHandle gBufferDepth,
    RGTextureHandle voxelRadiance3D,
    RGTextureHandle outIndirectDiffuse,
    const VoxelGIConfig& config
) {
    rg.AddPass("VoxelConeTracingPass",
        [&](RGPassBuilder& builder) {
            builder.ReadTexture(gBufferNormal, RHI::ResourceState::ShaderResource);
            builder.ReadTexture(gBufferDepth, RHI::ResourceState::ShaderResource);
            builder.ReadTexture(voxelRadiance3D, RHI::ResourceState::ShaderResource);
            builder.WriteTexture(outIndirectDiffuse, RHI::ResourceState::RenderTarget);
        },
        [config, outIndirectDiffuse](RHI::RHICommandList& cmdList, const RenderGraph& graph) {
            const auto* outTex = graph.GetPhysicalTexture(outIndirectDiffuse);
            if (outTex) {
                struct VoxelConstants {
                    float maxTracingDistance;
                    float volumeWorldExtent;
                    float bounceIntensity;
                    Core::u32 volumeResolution;
                } constants{config.maxTracingDistance, config.volumeWorldExtent, config.bounceIntensity, config.volumeResolution};

                cmdList.SetRoot32BitConstants(0, sizeof(VoxelConstants) / 4, &constants, 0);

                const Core::u32 threadsX = (outTex->GetWidth() + 7) / 8;
                const Core::u32 threadsY = (outTex->GetHeight() + 7) / 8;
                cmdList.DispatchCompute(threadsX, threadsY, 1);
            }
        }
    );
}

} // namespace Engine::Renderer
