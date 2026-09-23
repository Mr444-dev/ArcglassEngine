#pragma once

#include "engine/renderer/RenderGraph.h"

namespace Engine::Renderer {

struct VolumetricFogConfig {
    Core::u32 gridWidth{160};
    Core::u32 gridHeight{90};
    Core::u32 gridDepth{64};
    float scatteringG{0.4f}; // Anisotropy (-1.0 to 1.0)
    float density{0.02f};
    float nearZ{0.1f};
    float farZ{100.0f};
};

class VolumetricFogPass {
public:
    VolumetricFogPass() = default;

    static float CalculateSliceDepth(Core::u32 slice, Core::u32 totalSlices, float nearZ, float farZ) noexcept;
    static float EvaluatePhase(float cosTheta, float g) noexcept;

    static void AddToGraph(
        RenderGraph& rg,
        RGTextureHandle depthBuffer,
        RGTextureHandle outFogVolume,
        const VolumetricFogConfig& config = {}
    );
};

} // namespace Engine::Renderer
