#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/renderer/RenderGraph.h"
#include <array>
#include <vector>
#include <unordered_map>

namespace Engine::Renderer {

// --- Procedural Terrain Clipmap ---

struct ClipmapLevelDesc {
    Core::u32 levelIndex{0};
    float gridSpacing{1.0f};      // Spacing between grid vertices (meters)
    float ringExtent{64.0f};       // Half-extent of this square concentric ring
    Core::Vec2 centerOffset{0.0f, 0.0f};
};

class TerrainClipmap {
public:
    static constexpr size_t NUM_LOD_LEVELS = 6;
    static constexpr Core::u32 GRID_RESOLUTION = 64; // 64x64 quads per ring

    explicit TerrainClipmap(float baseSpacing = 0.5f, float baseExtent = 32.0f);

    // Updates concentric ring centers based on camera position, snapped to grid spacing
    void UpdateCameraPosition(const Core::Vec3& cameraPos) noexcept;

    // Computes continuous geomorphing blending factor alpha in [0, 1] to prevent vertex popping
    [[nodiscard]] static float ComputeGeomorphFactor(
        float distanceToCenter,
        float ringInnerRadius,
        float ringOuterRadius
    ) noexcept;

    // Evaluates procedural height at world coordinates (x, z)
    [[nodiscard]] static float SampleProceduralElevation(float worldX, float worldZ) noexcept;

    [[nodiscard]] const std::array<ClipmapLevelDesc, NUM_LOD_LEVELS>& GetLevels() const noexcept {
        return m_levels;
    }

    [[nodiscard]] Core::Vec3 GetLastCameraPos() const noexcept { return m_lastCameraPos; }

private:
    std::array<ClipmapLevelDesc, NUM_LOD_LEVELS> m_levels;
    Core::Vec3 m_lastCameraPos{0.0f, 0.0f, 0.0f};
};

// --- Sparse Virtual Texture (SVT) Tile Cache ---

struct VirtualPageID {
    Core::u16 level{0};
    Core::u16 x{0};
    Core::u16 y{0};

    bool operator==(const VirtualPageID& other) const noexcept {
        return level == other.level && x == other.x && y == other.y;
    }
};

struct VirtualPageHash {
    size_t operator()(const VirtualPageID& id) const noexcept {
        return (static_cast<size_t>(id.level) << 32) ^
               (static_cast<size_t>(id.x) << 16) ^
               (static_cast<size_t>(id.y));
    }
};

struct PhysicalTileCoord {
    Core::u16 tileX{0};
    Core::u16 tileY{0};
    bool isValid{false};
};

class VirtualTextureManager {
public:
    static constexpr Core::u32 VIRTUAL_TEXTURE_RESOLUTION = 65536; // 64k x 64k virtual texture
    static constexpr Core::u32 TILE_SIZE = 256;                   // 256 x 256 pixels per tile
    static constexpr Core::u32 NUM_VIRTUAL_PAGES_PER_AXIS = VIRTUAL_TEXTURE_RESOLUTION / TILE_SIZE; // 256
    static constexpr Core::u32 PHYSICAL_ATLAS_TILES_PER_AXIS = 16; // 16x16 = 256 physical slots (4096x4096 atlas)
    static constexpr size_t PHYSICAL_TILE_CAPACITY = PHYSICAL_ATLAS_TILES_PER_AXIS * PHYSICAL_ATLAS_TILES_PER_AXIS;

    VirtualTextureManager();

    // Requests a virtual page; returns physical tile coordinates if resident, or queues load
    [[nodiscard]] PhysicalTileCoord TouchPage(const VirtualPageID& pageId) noexcept;

    // Forces registration of a physical tile allocation for a page
    void AllocatePage(const VirtualPageID& pageId) noexcept;

    // Checks if a page is resident in physical GPU cache
    [[nodiscard]] bool IsPageResident(const VirtualPageID& pageId) const noexcept;

    [[nodiscard]] size_t GetResidentPageCount() const noexcept { return m_pageTable.size(); }
    [[nodiscard]] size_t GetPendingRequestsCount() const noexcept { return m_pendingRequests.size(); }
    [[nodiscard]] Core::u64 GetCacheHitCount() const noexcept { return m_cacheHits; }
    [[nodiscard]] Core::u64 GetCacheMissCount() const noexcept { return m_cacheMisses; }

    static void AddToGraph(
        RenderGraph& rg,
        RGTextureHandle gBufferDepth,
        RGTextureHandle terrainHeightmap,
        RGTextureHandle outTerrainOutput
    );

private:
    std::unordered_map<VirtualPageID, PhysicalTileCoord, VirtualPageHash> m_pageTable;
    std::vector<VirtualPageID> m_lruQueue; // Front = least recently used
    std::vector<VirtualPageID> m_pendingRequests;

    Core::u16 m_nextPhysicalSlot{0};
    Core::u64 m_cacheHits{0};
    Core::u64 m_cacheMisses{0};
};

} // namespace Engine::Renderer
