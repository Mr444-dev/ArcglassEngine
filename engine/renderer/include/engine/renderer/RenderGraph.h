#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/rhi/RHICommandList.h"
#include "engine/rhi/RHIDevice.h"
#include "engine/renderer/RenderGraphResource.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

namespace Engine::Renderer {

class RenderGraph;

class RGPassBuilder {
public:
    RGPassBuilder(RenderGraph& graph, Core::u32 passIndex);

    RGTextureHandle ReadTexture(RGTextureHandle handle, RHI::ResourceState state = RHI::ResourceState::ShaderResource);
    RGTextureHandle WriteTexture(RGTextureHandle handle, RHI::ResourceState state = RHI::ResourceState::RenderTarget);
    RGTextureHandle WriteDepthStencil(RGTextureHandle handle, RHI::ResourceState state = RHI::ResourceState::DepthWrite);

    RGBufferHandle ReadBuffer(RGBufferHandle handle, RHI::ResourceState state = RHI::ResourceState::ShaderResource);
    RGBufferHandle WriteBuffer(RGBufferHandle handle, RHI::ResourceState state = RHI::ResourceState::UnorderedAccess);

private:
    RenderGraph& m_graph;
    Core::u32 m_passIndex;
};

struct RGBarrier {
    RGTextureHandle textureHandle;
    RHI::ResourceState targetState;
};

struct RGPass {
    std::string name;
    Core::u32 index{0};

    std::vector<std::pair<RGTextureHandle, RHI::ResourceState>> textureReads;
    std::vector<std::pair<RGTextureHandle, RHI::ResourceState>> textureWrites;
    std::vector<std::pair<RGBufferHandle, RHI::ResourceState>> bufferReads;
    std::vector<std::pair<RGBufferHandle, RHI::ResourceState>> bufferWrites;

    RGTextureHandle depthStencilTarget{};
    std::vector<RGTextureHandle> renderTargets;

    std::vector<RGBarrier> prePassBarriers;

    std::function<void(RHI::RHICommandList&, const RenderGraph&)> executeCallback;
    bool culled{false};
    Core::u32 refCount{0};
};

class RenderGraph {
public:
    RenderGraph() = default;
    ~RenderGraph();

    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;

    // Resource Declaration
    RGTextureHandle DeclareTexture(const std::string& name, const RGTextureDesc& desc);
    RGTextureHandle ImportTexture(const std::string& name, RHI::RHITexture* externalTexture, RHI::ResourceState initialState = RHI::ResourceState::Common);
    RGBufferHandle DeclareBuffer(const std::string& name, const RGBufferDesc& desc);

    void MarkOutput(RGTextureHandle handle);
    void MarkOutput(RGBufferHandle handle);

    // Pass Declaration
    template <typename SetupFn, typename ExecFn>
    void AddPass(const std::string& name, SetupFn&& setup, ExecFn&& execute) {
        const Core::u32 passIndex = static_cast<Core::u32>(m_passes.size());
        auto& pass = m_passes.emplace_back();
        pass.name = name;
        pass.index = passIndex;
        pass.executeCallback = std::forward<ExecFn>(execute);

        RGPassBuilder builder(*this, passIndex);
        setup(builder);
    }

    // Compilation & Execution
    bool Compile();
    void Execute(RHI::RHICommandList& cmdList);
    void Reset();

    // Query physical resources
    [[nodiscard]] RHI::RHITexture* GetPhysicalTexture(RGTextureHandle handle) const;
    [[nodiscard]] RHI::RHIBuffer* GetPhysicalBuffer(RGBufferHandle handle) const;

    [[nodiscard]] size_t GetPassCount() const noexcept { return m_passes.size(); }
    [[nodiscard]] size_t GetCompiledPassCount() const noexcept { return m_compiledPassOrder.size(); }
    [[nodiscard]] bool IsCompiled() const noexcept { return m_isCompiled; }

private:
    friend class RGPassBuilder;

    void RealizePhysicalResources();
    bool TopologicalSort();
    void CullUnusedPasses();
    void ComputeResourceBarriers();

    std::vector<RGPass> m_passes;
    std::vector<Core::u32> m_compiledPassOrder;

    std::vector<RGTextureResource> m_textures;
    std::vector<RGBufferResource> m_buffers;

    // Transient physical resource cache
    std::vector<std::unique_ptr<RHI::RHITexture>> m_transientTexturePool;

    bool m_isCompiled{false};
};

} // namespace Engine::Renderer
