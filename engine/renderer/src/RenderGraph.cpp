#include "engine/renderer/RenderGraph.h"
#include <queue>
#include <algorithm>

namespace Engine::Renderer {

RGPassBuilder::RGPassBuilder(RenderGraph& graph, Core::u32 passIndex)
    : m_graph(graph)
    , m_passIndex(passIndex)
{
}

RGTextureHandle RGPassBuilder::ReadTexture(RGTextureHandle handle, RHI::ResourceState state) {
    ENGINE_ASSERT(handle.IsValid() && handle.id < m_graph.m_textures.size(), "Invalid RGTextureHandle for ReadTexture");
    auto& pass = m_graph.m_passes[m_passIndex];
    pass.textureReads.emplace_back(handle, state);
    m_graph.m_textures[handle.id].refCount++;
    return handle;
}

RGTextureHandle RGPassBuilder::WriteTexture(RGTextureHandle handle, RHI::ResourceState state) {
    ENGINE_ASSERT(handle.IsValid() && handle.id < m_graph.m_textures.size(), "Invalid RGTextureHandle for WriteTexture");
    auto& pass = m_graph.m_passes[m_passIndex];
    pass.textureWrites.emplace_back(handle, state);
    pass.renderTargets.push_back(handle);
    return handle;
}

RGTextureHandle RGPassBuilder::WriteDepthStencil(RGTextureHandle handle, RHI::ResourceState state) {
    ENGINE_ASSERT(handle.IsValid() && handle.id < m_graph.m_textures.size(), "Invalid RGTextureHandle for WriteDepthStencil");
    auto& pass = m_graph.m_passes[m_passIndex];
    pass.textureWrites.emplace_back(handle, state);
    pass.depthStencilTarget = handle;
    return handle;
}

RGBufferHandle RGPassBuilder::ReadBuffer(RGBufferHandle handle, RHI::ResourceState state) {
    ENGINE_ASSERT(handle.IsValid() && handle.id < m_graph.m_buffers.size(), "Invalid RGBufferHandle for ReadBuffer");
    auto& pass = m_graph.m_passes[m_passIndex];
    pass.bufferReads.emplace_back(handle, state);
    m_graph.m_buffers[handle.id].refCount++;
    return handle;
}

RGBufferHandle RGPassBuilder::WriteBuffer(RGBufferHandle handle, RHI::ResourceState state) {
    ENGINE_ASSERT(handle.IsValid() && handle.id < m_graph.m_buffers.size(), "Invalid RGBufferHandle for WriteBuffer");
    auto& pass = m_graph.m_passes[m_passIndex];
    pass.bufferWrites.emplace_back(handle, state);
    return handle;
}

RenderGraph::~RenderGraph() {
    Reset();
    m_transientTexturePool.clear();
}

RGTextureHandle RenderGraph::DeclareTexture(const std::string& name, const RGTextureDesc& desc) {
    const Core::u32 id = static_cast<Core::u32>(m_textures.size());
    auto& res = m_textures.emplace_back();
    res.desc = desc;
    res.desc.name = name;
    res.currentState = RHI::ResourceState::Common;
    res.isImported = false;
    res.refCount = 0;
    return RGTextureHandle{id, 0};
}

RGTextureHandle RenderGraph::ImportTexture(const std::string& name, RHI::RHITexture* externalTexture, RHI::ResourceState initialState) {
    ENGINE_ASSERT(externalTexture != nullptr, "Cannot import null RHITexture");
    const Core::u32 id = static_cast<Core::u32>(m_textures.size());
    auto& res = m_textures.emplace_back();
    res.desc.name = name;
    res.desc.width = externalTexture->GetWidth();
    res.desc.height = externalTexture->GetHeight();
    res.desc.format = externalTexture->GetFormat();
    res.physicalTexture = externalTexture;
    res.currentState = initialState;
    res.isImported = true;
    res.refCount = 1; // External resources have inherent side-effects
    return RGTextureHandle{id, 0};
}

RGBufferHandle RenderGraph::DeclareBuffer(const std::string& name, const RGBufferDesc& desc) {
    const Core::u32 id = static_cast<Core::u32>(m_buffers.size());
    auto& res = m_buffers.emplace_back();
    res.desc = desc;
    res.desc.name = name;
    res.currentState = RHI::ResourceState::Common;
    res.isImported = false;
    res.refCount = 0;
    return RGBufferHandle{id, 0};
}

void RenderGraph::MarkOutput(RGTextureHandle handle) {
    if (handle.IsValid() && handle.id < m_textures.size()) {
        m_textures[handle.id].isOutput = true;
        m_textures[handle.id].refCount++;
    }
}

void RenderGraph::MarkOutput(RGBufferHandle handle) {
    if (handle.IsValid() && handle.id < m_buffers.size()) {
        m_buffers[handle.id].isOutput = true;
        m_buffers[handle.id].refCount++;
    }
}

void RenderGraph::CullUnusedPasses() {
    // 1. Mark pass ref counts based on writes to resources with refCount > 0, imported or marked as output
    for (auto& pass : m_passes) {
        pass.refCount = 0;
        for (const auto& [texHandle, _] : pass.textureWrites) {
            const auto& res = m_textures[texHandle.id];
            if (res.isImported || res.isOutput || res.refCount > 0) {
                pass.refCount++;
            }
        }
        for (const auto& [bufHandle, _] : pass.bufferWrites) {
            const auto& res = m_buffers[bufHandle.id];
            if (res.isImported || res.isOutput || res.refCount > 0) {
                pass.refCount++;
            }
        }
        if (pass.refCount == 0) {
            pass.culled = true;
            ENGINE_LOG_TRACE("RenderGraph: Culled unused pass '{}'", pass.name);
        } else {
            pass.culled = false;
        }
    }
}

bool RenderGraph::TopologicalSort() {
    const size_t passCount = m_passes.size();
    std::vector<std::vector<Core::u32>> adj(passCount);
    std::vector<Core::u32> inDegree(passCount, 0);

    // Build dependency graph: Producer -> Consumer
    for (Core::u32 p = 0; p < passCount; ++p) {
        if (m_passes[p].culled) continue;

        // Texture dependencies
        for (const auto& [readTex, _] : m_passes[p].textureReads) {
            for (Core::u32 prod = 0; prod < passCount; ++prod) {
                if (prod == p || m_passes[prod].culled) continue;
                for (const auto& [writeTex, __] : m_passes[prod].textureWrites) {
                    if (writeTex == readTex) {
                        adj[prod].push_back(p);
                        inDegree[p]++;
                        break;
                    }
                }
            }
        }

        // Buffer dependencies
        for (const auto& [readBuf, _] : m_passes[p].bufferReads) {
            for (Core::u32 prod = 0; prod < passCount; ++prod) {
                if (prod == p || m_passes[prod].culled) continue;
                for (const auto& [writeBuf, __] : m_passes[prod].bufferWrites) {
                    if (writeBuf == readBuf) {
                        adj[prod].push_back(p);
                        inDegree[p]++;
                        break;
                    }
                }
            }
        }
    }

    // Kahn's Algorithm
    std::queue<Core::u32> q;
    for (Core::u32 i = 0; i < passCount; ++i) {
        if (!m_passes[i].culled && inDegree[i] == 0) {
            q.push(i);
        }
    }

    m_compiledPassOrder.clear();
    while (!q.empty()) {
        Core::u32 u = q.front();
        q.pop();
        m_compiledPassOrder.push_back(u);

        for (Core::u32 v : adj[u]) {
            if (--inDegree[v] == 0) {
                q.push(v);
            }
        }
    }

    // Check for circular dependency
    size_t activePasses = 0;
    for (const auto& p : m_passes) {
        if (!p.culled) activePasses++;
    }

    if (m_compiledPassOrder.size() < activePasses) {
        ENGINE_LOG_ERROR("RenderGraph: Circular dependency detected in render passes! Active: {}, Sorted: {}",
            activePasses, m_compiledPassOrder.size());
        return false;
    }

    return true;
}

void RenderGraph::RealizePhysicalResources() {
    auto& device = RHI::RHIDevice::Get();
    if (!device.GetNativeDevice()) {
        return; // GPU hardware device not yet initialized (e.g. pure graph analysis)
    }

    for (auto& res : m_textures) {
        if (res.isImported || res.physicalTexture != nullptr) continue;

        // Search in pool
        for (auto& pooled : m_transientTexturePool) {
            if (pooled && pooled->GetWidth() == res.desc.width &&
                pooled->GetHeight() == res.desc.height &&
                pooled->GetFormat() == res.desc.format) {
                res.physicalTexture = pooled.get();
                break;
            }
        }

        if (!res.physicalTexture) {
            RHI::TextureDesc tDesc{};
            tDesc.width = res.desc.width;
            tDesc.height = res.desc.height;
            tDesc.format = res.desc.format;
            tDesc.usage = res.desc.usage;
            tDesc.debugName = res.desc.name;

            auto newTex = device.CreateTexture(tDesc);
            res.physicalTexture = newTex.get();
            m_transientTexturePool.push_back(std::move(newTex));
        }
    }
}

void RenderGraph::ComputeResourceBarriers() {
    for (Core::u32 passIdx : m_compiledPassOrder) {
        auto& pass = m_passes[passIdx];
        pass.prePassBarriers.clear();

        // Check Texture Writes
        for (const auto& [texHandle, targetState] : pass.textureWrites) {
            auto& res = m_textures[texHandle.id];
            if (res.currentState != targetState) {
                pass.prePassBarriers.push_back(RGBarrier{texHandle, targetState});
                res.currentState = targetState;
            }
        }

        // Check Texture Reads
        for (const auto& [texHandle, targetState] : pass.textureReads) {
            auto& res = m_textures[texHandle.id];
            if (res.currentState != targetState) {
                pass.prePassBarriers.push_back(RGBarrier{texHandle, targetState});
                res.currentState = targetState;
            }
        }
    }
}

bool RenderGraph::Compile() {
    CullUnusedPasses();
    if (!TopologicalSort()) {
        m_isCompiled = false;
        return false;
    }
    RealizePhysicalResources();
    ComputeResourceBarriers();

    m_isCompiled = true;
    ENGINE_LOG_TRACE("RenderGraph successfully compiled: {} active passes scheduled.", m_compiledPassOrder.size());
    return true;
}

void RenderGraph::Execute(RHI::RHICommandList& cmdList) {
    ENGINE_ASSERT(m_isCompiled, "RenderGraph must be compiled before execution");

    for (Core::u32 passIdx : m_compiledPassOrder) {
        auto& pass = m_passes[passIdx];

        // 1. Submit barriers
        for (const auto& barrier : pass.prePassBarriers) {
            auto* phys = m_textures[barrier.textureHandle.id].physicalTexture;
            cmdList.TransitionBarrier(phys, barrier.targetState);
        }

        // 2. Bind Render Targets if configured
        if (!pass.renderTargets.empty() || pass.depthStencilTarget.IsValid()) {
            std::vector<RHI::RHITexture*> rts;
            rts.reserve(pass.renderTargets.size());
            for (auto h : pass.renderTargets) {
                rts.push_back(m_textures[h.id].physicalTexture);
            }

            RHI::RHITexture* dsv = pass.depthStencilTarget.IsValid() 
                ? m_textures[pass.depthStencilTarget.id].physicalTexture 
                : nullptr;

            cmdList.SetRenderTargets(static_cast<Core::u32>(rts.size()), rts.data(), dsv);
        }

        // 3. Execute pass logic
        if (pass.executeCallback) {
            pass.executeCallback(cmdList, *this);
        }
    }
}

void RenderGraph::Reset() {
    m_passes.clear();
    m_compiledPassOrder.clear();
    m_textures.clear();
    m_buffers.clear();
    m_isCompiled = false;
}

RHI::RHITexture* RenderGraph::GetPhysicalTexture(RGTextureHandle handle) const {
    if (!handle.IsValid() || handle.id >= m_textures.size()) return nullptr;
    return m_textures[handle.id].physicalTexture;
}

RHI::RHIBuffer* RenderGraph::GetPhysicalBuffer(RGBufferHandle handle) const {
    if (!handle.IsValid() || handle.id >= m_buffers.size()) return nullptr;
    return m_buffers[handle.id].physicalBuffer;
}

} // namespace Engine::Renderer
