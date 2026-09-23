#include "engine/renderer/passes/TAAPass.h"
#include <algorithm>

namespace Engine::Renderer {

float TAAPass::GetHalton(Core::u32 index, Core::u32 base) noexcept {
    float result = 0.0f;
    float f = 1.0f / static_cast<float>(base);
    Core::u32 i = index;

    while (i > 0) {
        result += f * static_cast<float>(i % base);
        i /= base;
        f /= static_cast<float>(base);
    }

    return result;
}

Core::Vec2 TAAPass::GetJitter(Core::u32 frameIndex, Core::u32 renderWidth, Core::u32 renderHeight) noexcept {
    if (renderWidth == 0 || renderHeight == 0) return {0.0f, 0.0f};

    // 16-phase Halton sequence (base 2 for X, base 3 for Y) centered around 0.0
    const Core::u32 sampleIndex = (frameIndex % 16) + 1;
    const float haltonX = GetHalton(sampleIndex, 2) - 0.5f;
    const float haltonY = GetHalton(sampleIndex, 3) - 0.5f;

    // Convert pixel offset to NDC subpixel shift
    return {
        (haltonX * 2.0f) / static_cast<float>(renderWidth),
        (haltonY * 2.0f) / static_cast<float>(renderHeight)
    };
}

Core::Vec3 TAAPass::ClipColor(
    const Core::Vec3& history,
    const Core::Vec3& mean,
    const Core::Vec3& stdDev,
    float gamma
) noexcept {
    const Core::Vec3 minBox = {
        mean.x - gamma * stdDev.x,
        mean.y - gamma * stdDev.y,
        mean.z - gamma * stdDev.z
    };
    const Core::Vec3 maxBox = {
        mean.x + gamma * stdDev.x,
        mean.y + gamma * stdDev.y,
        mean.z + gamma * stdDev.z
    };

    return {
        std::clamp(history.x, minBox.x, maxBox.x),
        std::clamp(history.y, minBox.y, maxBox.y),
        std::clamp(history.z, minBox.z, maxBox.z)
    };
}

void TAAPass::AddToGraph(
    RenderGraph& rg,
    RGTextureHandle currentFrame,
    RGTextureHandle historyFrame,
    RGTextureHandle outResolved
) {
    rg.AddPass("TAAPass",
        [&](RGPassBuilder& builder) {
            builder.ReadTexture(currentFrame, RHI::ResourceState::ShaderResource);
            builder.ReadTexture(historyFrame, RHI::ResourceState::ShaderResource);
            builder.WriteTexture(outResolved, RHI::ResourceState::RenderTarget);
        },
        [](RHI::RHICommandList& cmdList, const RenderGraph& graph) {
            (void)cmdList;
            (void)graph;
            // Dispatches TAA resolve fullscreen quad or compute shader
        }
    );
}

} // namespace Engine::Renderer
