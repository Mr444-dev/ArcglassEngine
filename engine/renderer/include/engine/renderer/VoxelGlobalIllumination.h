#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/renderer/RenderGraph.h"
#include <array>

namespace Engine::Renderer {

struct VoxelCone {
    Core::Vec3 direction;
    float aperture{0.577f}; // tan(30 deg) for a 60 deg cone
    float weight{1.0f};
};

struct VoxelGIConfig {
    Core::u32 volumeResolution{128};
    float volumeWorldExtent{64.0f}; // Covers 64 meters
    Core::Vec3 volumeCenter{0.0f, 0.0f, 0.0f};
    float maxTracingDistance{32.0f};
    float bounceIntensity{1.0f};
};

class VoxelGlobalIllumination {
public:
    VoxelGlobalIllumination() = default;

    // Generates 6 hemispherical diffuse cones aligned with surface normal
    [[nodiscard]] static std::array<VoxelCone, 6> GenerateDiffuseCones(const Core::Vec3& normal) noexcept;

    // Transforms world-space position to 3D voxel texture UVW coordinates [0, 1]
    [[nodiscard]] static Core::Vec3 WorldToVoxelUVW(
        const Core::Vec3& worldPos,
        const Core::Vec3& volumeCenter,
        float volumeExtent
    ) noexcept;

    static void AddToGraph(
        RenderGraph& rg,
        RGTextureHandle gBufferNormal,
        RGTextureHandle gBufferDepth,
        RGTextureHandle voxelRadiance3D,
        RGTextureHandle outIndirectDiffuse,
        const VoxelGIConfig& config = {}
    );
};

} // namespace Engine::Renderer
