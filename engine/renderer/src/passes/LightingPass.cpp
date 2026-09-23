#include "engine/renderer/passes/LightingPass.h"

namespace Engine::Renderer {

void LightingPass::AddToGraph(
    RenderGraph& graph,
    RGTextureHandle depthTarget,
    RGTextureHandle outColorTarget
) {
    graph.AddPass(
        "LightingPass (PBR Cook-Torrance)",
        [&](RGPassBuilder& builder) {
            builder.ReadTexture(depthTarget, RHI::ResourceState::DepthRead);
            builder.WriteTexture(outColorTarget, RHI::ResourceState::RenderTarget);
        },
        [=](RHI::RHICommandList& cmdList, const RenderGraph& rg) {
            auto* colorRT = rg.GetPhysicalTexture(outColorTarget);
            if (colorRT) {
                // Clear HDR scene color
                cmdList.ClearRenderTarget(colorRT, 0.12f, 0.16f, 0.28f, 1.0f);
            }
        }
    );
}

} // namespace Engine::Renderer
