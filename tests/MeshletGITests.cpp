#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/renderer/MeshletLODDAG.h"
#include "engine/renderer/VoxelGlobalIllumination.h"
#include "engine/renderer/RenderGraph.h"

void TestMeshletLODDAGCutSelection() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Meshlet Continuous LOD DAG Selection ---");

    MeshletCluster cluster{};
    cluster.clusterID = 101;
    cluster.lodLevel = 1;
    cluster.boundingSphereCenter = {0.0f, 0.0f, 10.0f};
    cluster.boundingSphereRadius = 1.0f;
    cluster.clusterError = 0.02f; // 2 cm world error
    cluster.parentError = 0.20f;  // 20 cm world error

    Vec3 cameraPos{0.0f, 0.0f, 0.0f};
    float fovY = 0.785398f; // 45 deg
    float screenHeight = 1080.0f;
    float thresholdPixels = 5.0f;

    // 1. At 10m:
    // Distance conservative = 10 - 1 = 9m
    // Projected own error: (0.02 * 1080) / (2 * 9 * tan(22.5 deg)) ~= 2.90 pixels (<= 5.0)
    // Projected parent error: (0.20 * 1080) / (2 * 9 * tan(22.5 deg)) ~= 29.0 pixels (> 5.0)
    // Should render!
    bool shouldRender = MeshletLODDAG::ShouldRenderCluster(cluster, cameraPos, thresholdPixels, fovY, screenHeight);
    ENGINE_ASSERT(shouldRender, "Cluster at 10m should be selected by DAG cut rule");

    // 2. Move camera very close (distance 1.5m -> error exceeds 5.0 pixels)
    Vec3 closeCameraPos{0.0f, 0.0f, 8.5f};
    bool renderClose = MeshletLODDAG::ShouldRenderCluster(cluster, closeCameraPos, thresholdPixels, fovY, screenHeight);
    ENGINE_ASSERT(!renderClose, "Cluster too close should not render (higher detail child needed)");

    // 3. Move camera far away (distance 100m -> parent error falls below 5.0 pixels)
    Vec3 farCameraPos{0.0f, 0.0f, -90.0f};
    bool renderFar = MeshletLODDAG::ShouldRenderCluster(cluster, farCameraPos, thresholdPixels, fovY, screenHeight);
    ENGINE_ASSERT(!renderFar, "Cluster too far should not render (coarser parent cluster needed)");

    ENGINE_LOG_INFO("Meshlet continuous LOD DAG cut selection rule verified across view distances.");
}

void TestProjectedErrorPerspectiveScaling() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Projected Error Perspective Scaling ---");

    float err10m = MeshletLODDAG::ProjectErrorToScreen(0.1f, 10.0f, 0.785f, 1080.0f);
    float err20m = MeshletLODDAG::ProjectErrorToScreen(0.1f, 20.0f, 0.785f, 1080.0f);

    // Doubling distance must strictly halve the projected screen space error
    float ratio = err10m / err20m;
    ENGINE_ASSERT(std::abs(ratio - 2.0f) < 1e-3f, "Perspective scaling must be inversely proportional to distance");

    ENGINE_LOG_INFO("Perspective screen error scaling verified (10m: {:.2f} px, 20m: {:.2f} px).", err10m, err20m);
}

void TestVoxelConeTracingHemisphere() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Voxel Cone Tracing Diffuse Cones ---");

    Vec3 normal{0.0f, 1.0f, 0.0f};
    auto cones = VoxelGlobalIllumination::GenerateDiffuseCones(normal);

    // Center cone points directly along normal
    ENGINE_ASSERT(cones[0].direction.Dot(normal) > 0.999f, "Center cone must align with normal");

    // All 5 side cones must have positive dot product (hemispherical coverage, cos(60) = 0.5)
    for (size_t i = 1; i < cones.size(); ++i) {
        float dotN = cones[i].direction.Dot(normal);
        ENGINE_ASSERT(std::abs(dotN - 0.5f) < 1e-3f, "Side cone elevation must be exactly 60 degrees");
        ENGINE_ASSERT(std::abs(cones[i].direction.Length() - 1.0f) < 1e-4f, "Cone direction must be normalized");
    }

    ENGINE_LOG_INFO("Voxel Cone Tracing 6-cone hemispherical distribution verified.");
}

void TestVoxelWorldToUVWCoordinates() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Voxel World To UVW Coordinates ---");

    Vec3 center{0.0f, 0.0f, 0.0f};
    float extent = 64.0f;

    // Center maps to (0.5, 0.5, 0.5)
    Vec3 uvwCenter = VoxelGlobalIllumination::WorldToVoxelUVW(center, center, extent);
    ENGINE_ASSERT(std::abs(uvwCenter.x - 0.5f) < 1e-4f && std::abs(uvwCenter.y - 0.5f) < 1e-4f, "Center UVW mismatch");

    // Min corner maps to (0, 0, 0)
    Vec3 minCorner{-32.0f, -32.0f, -32.0f};
    Vec3 uvwMin = VoxelGlobalIllumination::WorldToVoxelUVW(minCorner, center, extent);
    ENGINE_ASSERT(std::abs(uvwMin.x) < 1e-4f && std::abs(uvwMin.y) < 1e-4f, "Min corner UVW mismatch");

    // Max corner maps to (1, 1, 1)
    Vec3 maxCorner{32.0f, 32.0f, 32.0f};
    Vec3 uvwMax = VoxelGlobalIllumination::WorldToVoxelUVW(maxCorner, center, extent);
    ENGINE_ASSERT(std::abs(uvwMax.x - 1.0f) < 1e-4f && std::abs(uvwMax.y - 1.0f) < 1e-4f, "Max corner UVW mismatch");

    ENGINE_LOG_INFO("Voxel World to UVW [0, 1] projection verified.");
}

void TestRenderGraphVoxelConeTracingPass() {
    using namespace Engine::Renderer;
    using namespace Engine::RHI;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 14: Voxel Cone Tracing RenderGraph Integration ---");

    RenderGraph rg;

    RGTextureDesc normalDesc{1280, 720, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "GBufferNormal"};
    RGTextureHandle normal = rg.DeclareTexture("GBufferNormal", normalDesc);

    RGTextureDesc depthDesc{1280, 720, Format::D32_FLOAT, TextureUsage::DepthStencil, "GBufferDepth"};
    RGTextureHandle depth = rg.DeclareTexture("GBufferDepth", depthDesc);

    RGTextureDesc voxelDesc{128, 128, Format::RGBA16_FLOAT, TextureUsage::ShaderResource, "VoxelRadiance3D"};
    RGTextureHandle voxelRad = rg.DeclareTexture("VoxelRadiance3D", voxelDesc);

    RGTextureDesc indirectDesc{1280, 720, Format::RGBA16_FLOAT, TextureUsage::RenderTarget | TextureUsage::ShaderResource, "IndirectDiffuse"};
    RGTextureHandle indirect = rg.DeclareTexture("IndirectDiffuse", indirectDesc);

    VoxelGlobalIllumination::AddToGraph(rg, normal, depth, voxelRad, indirect);
    rg.MarkOutput(indirect);

    ENGINE_ASSERT(rg.Compile(), "Voxel Cone Tracing RenderGraph compilation failed");
    ENGINE_ASSERT(rg.GetCompiledPassCount() == 1, "Expected 1 active VCT pass scheduled");

    ENGINE_LOG_INFO("Voxel Cone Tracing RenderGraph pass successfully scheduled.");
}

void RunMeshletGIUnitTests() {
    TestMeshletLODDAGCutSelection();
    TestProjectedErrorPerspectiveScaling();
    TestVoxelConeTracingHemisphere();
    TestVoxelWorldToUVWCoordinates();
    TestRenderGraphVoxelConeTracingPass();
}
