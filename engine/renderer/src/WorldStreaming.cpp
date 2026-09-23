#include "engine/renderer/WorldStreaming.h"
#include <cmath>
#include <algorithm>

namespace Engine::Renderer {

// --- TerrainClipmap Implementation ---

TerrainClipmap::TerrainClipmap(float baseSpacing, float baseExtent) {
    float spacing = baseSpacing;
    float extent = baseExtent;

    for (size_t i = 0; i < NUM_LOD_LEVELS; ++i) {
        m_levels[i].levelIndex = static_cast<Core::u32>(i);
        m_levels[i].gridSpacing = spacing;
        m_levels[i].ringExtent = extent;
        m_levels[i].centerOffset = {0.0f, 0.0f};

        spacing *= 2.0f;
        extent *= 2.0f;
    }
}

void TerrainClipmap::UpdateCameraPosition(const Core::Vec3& cameraPos) noexcept {
    m_lastCameraPos = cameraPos;

    for (auto& level : m_levels) {
        // Snap center to grid spacing to eliminate texture/vertex shimmering
        const float snap = level.gridSpacing;
        level.centerOffset.x = std::floor(cameraPos.x / snap) * snap;
        level.centerOffset.y = std::floor(cameraPos.z / snap) * snap;
    }
}

float TerrainClipmap::ComputeGeomorphFactor(
    float distanceToCenter,
    float ringInnerRadius,
    float ringOuterRadius) noexcept {
    if (ringOuterRadius <= ringInnerRadius) return 0.0f;
    if (distanceToCenter <= ringInnerRadius) return 0.0f;
    if (distanceToCenter >= ringOuterRadius) return 1.0f;

    return (distanceToCenter - ringInnerRadius) / (ringOuterRadius - ringInnerRadius);
}

float TerrainClipmap::SampleProceduralElevation(float worldX, float worldZ) noexcept {
    // Multi-octave procedural fractal landscape synthesis
    const float o1 = std::sin(worldX * 0.015f) * std::cos(worldZ * 0.015f) * 25.0f;
    const float o2 = std::sin(worldX * 0.05f) * std::cos(worldZ * 0.05f) * 8.0f;
    const float o3 = std::sin(worldX * 0.15f + 1.2f) * std::cos(worldZ * 0.15f + 0.8f) * 2.0f;
    return o1 + o2 + o3;
}

// --- VirtualTextureManager Implementation ---

VirtualTextureManager::VirtualTextureManager() = default;

PhysicalTileCoord VirtualTextureManager::TouchPage(const VirtualPageID& pageId) noexcept {
    auto it = m_pageTable.find(pageId);
    if (it != m_pageTable.end()) {
        m_cacheHits++;
        // Refresh LRU order: move page to back
        auto lruIt = std::find(m_lruQueue.begin(), m_lruQueue.end(), pageId);
        if (lruIt != m_lruQueue.end()) {
            m_lruQueue.erase(lruIt);
            m_lruQueue.push_back(pageId);
        }
        return it->second;
    }

    m_cacheMisses++;
    m_pendingRequests.push_back(pageId);
    AllocatePage(pageId);
    return m_pageTable[pageId];
}

void VirtualTextureManager::AllocatePage(const VirtualPageID& pageId) noexcept {
    PhysicalTileCoord coord;
    coord.isValid = true;

    if (m_pageTable.size() >= PHYSICAL_TILE_CAPACITY && !m_lruQueue.empty()) {
        // Evict LRU page
        const VirtualPageID oldest = m_lruQueue.front();
        m_lruQueue.erase(m_lruQueue.begin());

        coord = m_pageTable[oldest];
        m_pageTable.erase(oldest);
    } else {
        // Allocate next sequential slot in the physical atlas
        coord.tileX = static_cast<Core::u16>(m_nextPhysicalSlot % PHYSICAL_ATLAS_TILES_PER_AXIS);
        coord.tileY = static_cast<Core::u16>(m_nextPhysicalSlot / PHYSICAL_ATLAS_TILES_PER_AXIS);
        m_nextPhysicalSlot = static_cast<Core::u16>((m_nextPhysicalSlot + 1) % PHYSICAL_TILE_CAPACITY);
    }

    m_pageTable[pageId] = coord;
    m_lruQueue.push_back(pageId);
}

bool VirtualTextureManager::IsPageResident(const VirtualPageID& pageId) const noexcept {
    return m_pageTable.find(pageId) != m_pageTable.end();
}

void VirtualTextureManager::AddToGraph(
    RenderGraph& rg,
    RGTextureHandle gBufferDepth,
    RGTextureHandle terrainHeightmap,
    RGTextureHandle outTerrainOutput) {
    rg.AddPass("TerrainStreamingPass",
        [&](RGPassBuilder& builder) {
            builder.ReadTexture(gBufferDepth, RHI::ResourceState::ShaderResource);
            builder.ReadTexture(terrainHeightmap, RHI::ResourceState::ShaderResource);
            builder.WriteTexture(outTerrainOutput, RHI::ResourceState::RenderTarget);
        },
        [outTerrainOutput](RHI::RHICommandList& cmdList, const RenderGraph& graph) {
            const auto* outTex = graph.GetPhysicalTexture(outTerrainOutput);
            if (outTex) {
                const Core::u32 threadsX = (outTex->GetWidth() + 7) / 8;
                const Core::u32 threadsY = (outTex->GetHeight() + 7) / 8;
                cmdList.DispatchCompute(threadsX, threadsY, 1);
            }
        }
    );
}

} // namespace Engine::Renderer
