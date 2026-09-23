#pragma once

#include "engine/renderer/RenderGraph.h"
#include "engine/core/MathTypes.h"

namespace Engine::Renderer {

class TAAPass {
public:
    TAAPass() = default;

    // Van der Corput radical inverse generator for Halton sequence
    [[nodiscard]] static float GetHalton(Core::u32 index, Core::u32 base) noexcept;

    // Subpixel jitter offset in normalized device coordinates (NDC)
    [[nodiscard]] static Core::Vec2 GetJitter(Core::u32 frameIndex, Core::u32 renderWidth, Core::u32 renderHeight) noexcept;

    // Variance clipping of history color against local 3x3 neighborhood
    [[nodiscard]] static Core::Vec3 ClipColor(
        const Core::Vec3& history,
        const Core::Vec3& mean,
        const Core::Vec3& stdDev,
        float gamma = 1.0f
    ) noexcept;

    static void AddToGraph(
        RenderGraph& rg,
        RGTextureHandle currentFrame,
        RGTextureHandle historyFrame,
        RGTextureHandle outResolved
    );
};

} // namespace Engine::Renderer
