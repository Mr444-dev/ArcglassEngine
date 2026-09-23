#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/renderer/WorldStreaming.h"
#include "engine/renderer/RenderGraph.h"

void TestTerrainClipmapSnappingAndLOD() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Terrain Clipmap Geometric Snapping & Concentric Rings ---");

    TerrainClipmap clipmap(1.0f, 32.0f);
    const auto& levels = clipmap.GetLevels();

    ENGINE_ASSERT(levels.size() == TerrainClipmap::NUM_LOD_LEVELS, "Terrain clipmap must contain exactly 6 LOD levels");

    // Verify spacing doubling per ring level
    float expectedSpacing = 1.0f;
    float expectedExtent = 32.0f;
    for (size_t i = 0; i < levels.size(); ++i) {
        ENGINE_ASSERT(std::abs(levels[i].gridSpacing - expectedSpacing) < 1e-4f, "Grid spacing must double per LOD level");
        ENGINE_ASSERT(std::abs(levels[i].ringExtent - expectedExtent) < 1e-4f, "Ring extent must double per LOD level");
        expectedSpacing *= 2.0f;
        expectedExtent *= 2.0f;
    }

    // Camera moves to arbitrary non-grid aligned point: (15.73, 5.0, 42.89)
    Vec3 cameraPos{15.73f, 5.0f, 42.89f};
    clipmap.UpdateCameraPosition(cameraPos);

    // Level 0 (spacing 1.0) -> centerOffset must be floor(15.73/1.0)*1.0 = 15.0, floor(42.89/1.0)*1.0 = 42.0
    ENGINE_ASSERT(std::abs(levels[0].centerOffset.x - 15.0f) < 1e-4f, "Level 0 X center must snap to 15.0");
    ENGINE_ASSERT(std::abs(levels[0].centerOffset.y - 42.0f) < 1e-4f, "Level 0 Z center must snap to 42.0");

    // Level 2 (spacing 4.0) -> floor(15.73/4.0)*4.0 = 12.0, floor(42.89/4.0)*4.0 = 40.0
    ENGINE_ASSERT(std::abs(levels[2].centerOffset.x - 12.0f) < 1e-4f, "Level 2 X center must snap to 12.0");
    ENGINE_ASSERT(std::abs(levels[2].centerOffset.y - 40.0f) < 1e-4f, "Level 2 Z center must snap to 40.0");

    ENGINE_LOG_INFO("Terrain clipmap concentric rings and grid snapping verified (LOD spacing 1.0m to 32.0m).");
}

void TestGeomorphFactorContinuity() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Continuous Geomorph Factor (Zero Popping) ---");

    const float innerRadius = 24.0f;
    const float outerRadius = 32.0f;

    // Inside inner boundary: morph factor must be strictly 0.0 (full resolution)
    float factorInside = TerrainClipmap::ComputeGeomorphFactor(10.0f, innerRadius, outerRadius);
    ENGINE_ASSERT(factorInside == 0.0f, "Inside inner radius must have 0.0 morph factor");
    (void)factorInside;

    // Outside outer boundary: morph factor must be strictly 1.0 (morphed to coarser level)
    float factorOutside = TerrainClipmap::ComputeGeomorphFactor(40.0f, innerRadius, outerRadius);
    ENGINE_ASSERT(factorOutside == 1.0f, "Outside outer radius must have 1.0 morph factor");
    (void)factorOutside;

    // Exactly midway (28.0m): must be 0.5 linear blend
    float factorMid = TerrainClipmap::ComputeGeomorphFactor(28.0f, innerRadius, outerRadius);
    ENGINE_ASSERT(std::abs(factorMid - 0.5f) < 1e-4f, "Midway morph factor must be 0.5");
    (void)factorMid;

    ENGINE_LOG_INFO("Continuous geomorphing alpha factor verified in [0.0, 1.0] transition zone.");
}

void TestProceduralElevationSampling() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Multi-Octave Procedural Elevation Synthesis ---");

    float h0 = TerrainClipmap::SampleProceduralElevation(0.0f, 0.0f);
    float h1 = TerrainClipmap::SampleProceduralElevation(150.0f, 300.0f);
    float h2 = TerrainClipmap::SampleProceduralElevation(150.0f, 300.0f);

    // Height must be deterministic
    ENGINE_ASSERT(h1 == h2, "Procedural elevation must be 100% deterministic for identical coordinates");
    (void)h2;
    ENGINE_ASSERT(h0 != h1, "Procedural elevation must vary across landscape coordinates");

    ENGINE_LOG_INFO("Multi-octave procedural fractal landscape elevation verified (H0: %.2f m, H1: %.2f m).", h0, h1);
}

void TestVirtualTextureTileCachingAndLRU() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Sparse Virtual Texture (SVT) LRU Page Table ---");

    VirtualTextureManager svt;

    // 1. Initial touch must result in cache miss and page allocation
    VirtualPageID p0{0, 10, 20};
    auto coord0 = svt.TouchPage(p0);
    ENGINE_ASSERT(coord0.isValid, "Allocated tile must be marked valid");
    ENGINE_ASSERT(svt.IsPageResident(p0), "Page p0 must be resident in physical GPU cache");
    ENGINE_ASSERT(svt.GetCacheMissCount() == 1, "First access must be a cache miss");

    // 2. Second touch must be a cache hit with identical physical coordinates
    auto coord0_hit = svt.TouchPage(p0);
    ENGINE_ASSERT(coord0_hit.tileX == coord0.tileX && coord0_hit.tileY == coord0.tileY, "Cache hit must return identical physical tile coords");
    ENGINE_ASSERT(svt.GetCacheHitCount() == 1, "Second access must be a cache hit");

    // 3. Fill up to capacity (256 physical slots = 16x16 atlas)
    for (u16 i = 1; i < VirtualTextureManager::PHYSICAL_TILE_CAPACITY; ++i) {
        VirtualPageID p{0, i, 0};
        auto c = svt.TouchPage(p);
        (void)c;
    }
    ENGINE_ASSERT(svt.GetResidentPageCount() == VirtualTextureManager::PHYSICAL_TILE_CAPACITY, "SVT cache must be at full capacity");

    // 4. Touching a new page beyond capacity must evict the least recently used page
    VirtualPageID overflowPage{1, 99, 99};
    auto cOverflow = svt.TouchPage(overflowPage);
    (void)cOverflow;
    ENGINE_ASSERT(svt.GetResidentPageCount() == VirtualTextureManager::PHYSICAL_TILE_CAPACITY, "Capacity must remain strictly clamped at 256 tiles");
    ENGINE_ASSERT(svt.IsPageResident(overflowPage), "New overflow page must be resident");

    ENGINE_LOG_INFO("Sparse Virtual Texture LRU caching, physical atlas wrapping (256 tiles) and eviction verified.");
}

void TestWorldStreamingRenderGraphPass() {
    using namespace Engine::Renderer;
    using namespace Engine::RHI;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing World Streaming RenderGraph Pass Scheduling ---");

    RenderGraph rg;

    RGTextureDesc depthDesc{1920, 1080, Format::D32_FLOAT, TextureUsage::DepthStencil, "GBufferDepth"};
    auto hDepth = rg.DeclareTexture("GBufferDepth", depthDesc);

    RGTextureDesc heightDesc{1024, 1024, Format::R32_FLOAT, TextureUsage::ShaderResource, "TerrainHeightmap"};
    auto hHeight = rg.DeclareTexture("TerrainHeightmap", heightDesc);

    RGTextureDesc outDesc{1920, 1080, Format::RGBA8_UNORM, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "TerrainSceneColor"};
    auto hTerrainOut = rg.DeclareTexture("TerrainSceneColor", outDesc);

    VirtualTextureManager::AddToGraph(rg, hDepth, hHeight, hTerrainOut);
    rg.MarkOutput(hTerrainOut);

    ENGINE_ASSERT(rg.Compile(), "RenderGraph compilation with TerrainStreamingPass must succeed");
    ENGINE_ASSERT(rg.GetCompiledPassCount() == 1, "Expected 1 active TerrainStreamingPass scheduled");

    ENGINE_LOG_INFO("TerrainStreamingPass RenderGraph node compiled and dependency graph validated.");
}

void RunWorldStreamingUnitTests() {
    TestTerrainClipmapSnappingAndLOD();
    TestGeomorphFactorContinuity();
    TestProceduralElevationSampling();
    TestVirtualTextureTileCachingAndLRU();
    TestWorldStreamingRenderGraphPass();
}
