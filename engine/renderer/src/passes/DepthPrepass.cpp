#include "engine/renderer/passes/DepthPrepass.h"

namespace Engine::Renderer {

void DepthPrepass::AddToGraph(
    RenderGraph& graph,
    RGTextureHandle depthTarget
) {
    graph.AddPass(
        "DepthPrepass (Early Z)",
        [&](RGPassBuilder& builder) {
            builder.WriteDepthStencil(depthTarget, RHI::ResourceState::DepthWrite);
        },
        [=](RHI::RHICommandList& cmdList, const RenderGraph& rg) {
            auto* dsv = rg.GetPhysicalTexture(depthTarget);
            if (dsv) {
                cmdList.ClearDepthStencil(dsv, 1.0f, 0);
            }
        }
    );
}

} // namespace Engine::Renderer
