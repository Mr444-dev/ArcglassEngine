#include "engine/renderer/passes/PostProcessPass.h"
#include "engine/core/Log.h"

namespace Engine::Renderer {

void PostProcessPass::AddToGraph(
    RenderGraph& graph,
    RGTextureHandle sceneColor,
    RGTextureHandle outputTarget
) {
    graph.AddPass(
        "PostProcessPass (ACES Tonemapping)",
        [&](RGPassBuilder& builder) {
            builder.ReadTexture(sceneColor, RHI::ResourceState::ShaderResource);
            builder.WriteTexture(outputTarget, RHI::ResourceState::RenderTarget);
        },
        [=](RHI::RHICommandList& cmdList, const RenderGraph& rg) {
            auto* target = rg.GetPhysicalTexture(outputTarget);
            if (target) {
                // Emulate fullscreen post-process blit & tonemap clear
                cmdList.ClearRenderTarget(target, 0.05f, 0.05f, 0.08f, 1.0f);
            }
        }
    );
}

} // namespace Engine::Renderer
