#pragma once

#include "engine/renderer/RenderGraph.h"

namespace Engine::Renderer {

class LightingPass {
public:
    static void AddToGraph(
        RenderGraph& graph,
        RGTextureHandle depthTarget,
        RGTextureHandle outColorTarget
    );
};

} // namespace Engine::Renderer
