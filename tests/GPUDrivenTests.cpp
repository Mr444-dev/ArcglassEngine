#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/GPUDrivenPipeline.h"
#include "engine/renderer/HierarchicalDepthBuffer.h"
#include "engine/renderer/CascadedShadowMaps.h"
#include <chrono>

void Test100000InstancesFrustumCullingAndLOD() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 7: 100,000 Instances GPU Frustum Culling & LOD ---");

    Camera camera;
    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetTarget({0.0f, 0.0f, 100.0f});
    camera.SetLens(0.785398f, 16.0f / 9.0f, 0.1f, 500.0f);
    camera.UpdateMatrices();

    GPUDrivenPipeline pipeline;
    constexpr size_t INSTANCE_COUNT = 100000;

    std::vector<GPUInstanceData> instances;
    instances.reserve(INSTANCE_COUNT);

    // 50,000 instances in front of camera (Z: 10 to 300)
    for (size_t i = 0; i < 50000; ++i) {
        GPUInstanceData inst{};
        const float z = 10.0f + static_cast<float>(i % 290);
        const float x = (static_cast<float>(i % 40) - 20.0f) * 0.5f;
        inst.worldMatrix = Mat4x4::Translation({x, 0.0f, z});
        inst.aabbMin = {-0.5f, -0.5f, -0.5f};
        inst.aabbMax = {0.5f, 0.5f, 0.5f};
        inst.indexCount = 1000;
        inst.meshID = static_cast<u32>(i % 5);
        inst.materialID = static_cast<u32>(i % 10);
        instances.push_back(inst);
    }

    // 50,000 instances behind the camera (Z: -10 to -300)
    for (size_t i = 0; i < 50000; ++i) {
        GPUInstanceData inst{};
        const float z = -10.0f - static_cast<float>(i % 290);
        inst.worldMatrix = Mat4x4::Translation({0.0f, 0.0f, z});
        inst.aabbMin = {-0.5f, -0.5f, -0.5f};
        inst.aabbMax = {0.5f, 0.5f, 0.5f};
        inst.indexCount = 1000;
        instances.push_back(inst);
    }

    pipeline.SetInstances(std::move(instances));
    ENGINE_ASSERT(pipeline.GetTotalInstanceCount() == INSTANCE_COUNT, "Instance count mismatch");

    std::vector<IndirectDrawCommand> outCommands;

    auto tStart = std::chrono::high_resolution_clock::now();
    pipeline.CullAndGenerateCommands(camera, 80.0f, 200.0f, outCommands);
    auto tEnd = std::chrono::high_resolution_clock::now();

    double cullTimeMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    ENGINE_LOG_INFO("Culled 100,000 instances in {:.2f} ms (Visible: {}, Culled: {}).",
        cullTimeMs, pipeline.GetVisibleInstanceCount(), INSTANCE_COUNT - pipeline.GetVisibleInstanceCount());

    ENGINE_ASSERT(pipeline.GetVisibleInstanceCount() > 0, "Expected visible instances");
    ENGINE_ASSERT(pipeline.GetVisibleInstanceCount() <= 50000, "Frustum culling should discard instances behind camera");
    ENGINE_ASSERT(!outCommands.empty(), "Indirect commands not generated");

    // Verify LOD reduction on distant commands
    bool hasReducedLOD = false;
    for (const auto& cmd : outCommands) {
        if (cmd.indexCountPerInstance < 1000) {
            hasReducedLOD = true;
            break;
        }
    }
    ENGINE_ASSERT(hasReducedLOD, "Distance-based LOD selection failed to reduce index count on distant instances");

    ENGINE_LOG_INFO("100,000 instances GPU-driven culling and LOD selection passed.");
}

void TestHierarchicalDepthBuffer() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Hierarchical Depth Buffer (HZB) ---");

    constexpr u32 WIDTH = 64;
    constexpr u32 HEIGHT = 64;
    std::vector<float> depths(WIDTH * HEIGHT, 1.0f); // Default far depth

    // Draw an occluder box at depth 0.2 in center (20..44)
    for (u32 y = 20; y < 44; ++y) {
        for (u32 x = 20; x < 44; ++x) {
            depths[y * WIDTH + x] = 0.2f;
        }
    }

    HierarchicalDepthBuffer hzb;
    hzb.BuildFromDepth(WIDTH, HEIGHT, depths.data());

    ENGINE_ASSERT(hzb.GetMipCount() == 7, "Expected 7 MIP levels for 64x64 HZB");
    ENGINE_ASSERT(hzb.GetMip(0)->depths.size() == 64 * 64, "HZB mip 0 size mismatch");

    Camera camera;
    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetTarget({0.0f, 0.0f, 100.0f});
    camera.SetLens(0.785398f, 1.0f, 0.1f, 100.0f);
    camera.UpdateMatrices();

    // AABB behind the occluder (Z = 80 -> depth ~0.8)
    AABB occludedAABB{{-0.5f, -0.5f, 79.5f}, {0.5f, 0.5f, 80.5f}};
    bool isOccluded = hzb.IsAABBOccluded(occludedAABB, camera);
    ENGINE_ASSERT(isOccluded, "Object behind occluder should be marked occluded");

    // AABB in front of occluder (Z = 5 -> depth ~0.05)
    AABB visibleAABB{{-0.5f, -0.5f, 4.5f}, {0.5f, 0.5f, 5.5f}};
    bool isVisible = !hzb.IsAABBOccluded(visibleAABB, camera);
    ENGINE_ASSERT(isVisible, "Object in front of occluder should remain visible");

    ENGINE_LOG_INFO("Hierarchical Depth Buffer (HZB) verified (7 MIPs, occlusion culling verified).");
}

void TestCascadedShadowMaps() {
    using namespace Engine::Renderer;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Cascaded Shadow Maps (CSM) ---");

    Camera camera;
    camera.SetPosition({0.0f, 2.0f, -10.0f});
    camera.SetTarget({0.0f, 0.0f, 0.0f});
    camera.SetLens(0.785398f, 16.0f / 9.0f, 0.1f, 200.0f);
    camera.UpdateMatrices();

    CascadedShadowMaps csm;
    csm.Update(camera, {0.577f, -0.577f, 0.577f}, 150.0f, 0.85f);

    ENGINE_ASSERT(csm.GetCascadeCount() == 4, "Expected 4 shadow cascades");

    // Verify monotonically increasing split distances
    float prevSplit = 0.0f;
    for (size_t i = 0; i < csm.GetCascadeCount(); ++i) {
        const auto& cascade = csm.GetCascade(i);
        ENGINE_ASSERT(cascade.splitDistance > prevSplit, "Cascade split distance must increase monotonically");
        prevSplit = cascade.splitDistance;
    }

    ENGINE_LOG_INFO("Cascaded Shadow Maps verified (4 cascades, splits: [{:.1f}, {:.1f}, {:.1f}, {:.1f}]).",
        csm.GetCascade(0).splitDistance,
        csm.GetCascade(1).splitDistance,
        csm.GetCascade(2).splitDistance,
        csm.GetCascade(3).splitDistance);
}

void RunGPUDrivenUnitTests() {
    Test100000InstancesFrustumCullingAndLOD();
    TestHierarchicalDepthBuffer();
    TestCascadedShadowMaps();
}
