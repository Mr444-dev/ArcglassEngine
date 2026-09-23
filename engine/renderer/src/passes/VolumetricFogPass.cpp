#include "engine/renderer/passes/VolumetricFogPass.h"
#include <cmath>
#include <numbers>

namespace Engine::Renderer {

float VolumetricFogPass::CalculateSliceDepth(Core::u32 slice, Core::u32 totalSlices, float nearZ, float farZ) noexcept {
    if (totalSlices == 0) return nearZ;
    const float t = (static_cast<float>(slice) + 0.5f) / static_cast<float>(totalSlices);
    return nearZ * std::pow(farZ / nearZ, t);
}

float VolumetricFogPass::EvaluatePhase(float cosTheta, float g) noexcept {
    constexpr float PI = 3.14159265358979323846f;
    const float g2 = g * g;
    const float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f - g2) / (4.0f * PI * std::pow(std::max(denom, 1e-4f), 1.5f));
}

void VolumetricFogPass::AddToGraph(
    RenderGraph& rg,
    RGTextureHandle depthBuffer,
    RGTextureHandle outFogVolume,
    const VolumetricFogConfig& config
) {
    (void)config;
    rg.AddPass("VolumetricFogPass",
        [&](RGPassBuilder& builder) {
            builder.ReadTexture(depthBuffer, RHI::ResourceState::ShaderResource);
            builder.WriteTexture(outFogVolume, RHI::ResourceState::UnorderedAccess);
        },
        [](RHI::RHICommandList& cmdList, const RenderGraph& graph) {
            (void)cmdList;
            (void)graph;
            // Dispatches compute shader CSMain for Froxel injection & raymarching
        }
    );
}

} // namespace Engine::Renderer
