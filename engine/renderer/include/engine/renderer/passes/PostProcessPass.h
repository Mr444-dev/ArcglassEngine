#pragma once

#include "engine/renderer/RenderGraph.h"

namespace Engine::Renderer {

class PostProcessPass {
public:
    PostProcessPass() = default;
    ~PostProcessPass() = default;

    static void AddToGraph(
        RenderGraph& graph,
        RGTextureHandle sceneColor,
        RGTextureHandle outputTarget
    );
};

} // namespace Engine::Renderer
