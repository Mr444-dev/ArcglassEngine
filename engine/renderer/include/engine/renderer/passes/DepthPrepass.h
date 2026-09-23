#pragma once

#include "engine/renderer/RenderGraph.h"

namespace Engine::Renderer {

class DepthPrepass {
public:
    static void AddToGraph(
        RenderGraph& graph,
        RGTextureHandle depthTarget
    );
};

} // namespace Engine::Renderer
