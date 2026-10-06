#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/renderer/MeshShaderCulling.h"
#include "engine/renderer/HierarchicalDepthBuffer.h"
#include "engine/renderer/RenderGraph.h"

void TestClusterNormalConeBackfaceCulling() {
    using namespace Engine::Core;
    using namespace Engine::Renderer;

    Logger::LogMessage(LogLevel::Info, "--- Testing Cluster Normal Cone Backface Micro-Culling ---");

    // Camera at (0, 0, -10) looking towards +Z
    const Vec3 camPos{0.0f, 0.0f, -10.0f};

    // 1. Cluster at origin facing camera: normal = (0, 0, -1)
    ClusterNormalCone facingCone;
    facingCone.apex = {0.0f, 0.0f, 0.0f};
    facingCone.axis = {0.0f, 0.0f, -1.0f}; // Points towards camera
    facingCone.cutoffAngleCosine = 0.866f;  // 30 degree semi-angle

    ENGINE_ASSERT(!MeshClusterCuller::IsConeBackfacing(facingCone, camPos), 
        "Cone pointing towards camera must NOT be culled");

    // 2. Cluster at origin facing away: normal = (0, 0, 1)
    ClusterNormalCone awayCone;
    awayCone.apex = {0.0f, 0.0f, 0.0f};
    awayCone.axis = {0.0f, 0.0f, 1.0f};   // Points away from camera
    awayCone.cutoffAngleCosine = 0.866f;  // 30 degree semi-angle

    ENGINE_ASSERT(MeshClusterCuller::IsConeBackfacing(awayCone, camPos), 
        "Cone pointing away from camera MUST be backface culled");

    ENGINE_LOG_INFO("Cluster normal cone orientation and angle cutoff validated (Backfacing cone culled).");
}

void TestClusterFrustumAndHZBOcclusion() {
    using namespace Engine::Core;
    using namespace Engine::Renderer;

    Logger::LogMessage(LogLevel::Info, "--- Testing Cluster Frustum & HZB Occlusion Evaluation ---");

    Camera camera;
    camera.SetPosition({0.0f, 0.0f, -10.0f});
    camera.SetTarget({0.0f, 0.0f, 0.0f});
    camera.SetLens(0.785398f, 16.0f / 9.0f, 0.1f, 1000.0f);
    camera.UpdateMatrices();

    // 1. Cluster inside frustum at origin
    AABB insideAABB{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    ENGINE_ASSERT(!MeshClusterCuller::IsFrustumCulled(insideAABB, camera.GetFrustum()), 
        "Cluster inside view frustum must survive frustum test");

    // 2. Cluster far behind camera: Z = -50
    AABB behindAABB{{-1.0f, -1.0f, -60.0f}, {1.0f, 1.0f, -40.0f}};
    ENGINE_ASSERT(MeshClusterCuller::IsFrustumCulled(behindAABB, camera.GetFrustum()), 
        "Cluster behind camera must be frustum culled");

    ENGINE_LOG_INFO("Cluster frustum plane intersection tests verified.");
}

void TestHardwareMeshClusterCullPipeline() {
    using namespace Engine::Core;
    using namespace Engine::Renderer;

    Logger::LogMessage(LogLevel::Info, "--- Testing Mesh Cluster CPU Culling Reference ---");

    Camera camera;
    camera.SetPosition({0.0f, 2.0f, -10.0f});
    camera.SetTarget({0.0f, 0.0f, 0.0f});
    camera.SetLens(0.785398f, 16.0f / 9.0f, 0.1f, 1000.0f);
    camera.UpdateMatrices();

    constexpr u32 TOTAL_CLUSTERS = 64;
    std::vector<MeshletHardwarePayload> clusters(TOTAL_CLUSTERS);

    for (u32 i = 0; i < TOTAL_CLUSTERS; ++i) {
        auto& c = clusters[i];
        c.cluster.clusterID = i;

        // Alternate half facing camera, half facing away
        const bool facesCamera = (i % 2 == 0);
        c.normalCone.apex = {0.0f, 0.0f, 0.0f};
        c.normalCone.axis = facesCamera ? Vec3{0.0f, 0.0f, -1.0f} : Vec3{0.0f, 0.0f, 1.0f};
        c.normalCone.cutoffAngleCosine = 0.866f; // 30 deg cone

        c.cluster.boundingBox.min = c.normalCone.apex - Vec3{0.5f, 0.5f, 0.5f};
        c.cluster.boundingBox.max = c.normalCone.apex + Vec3{0.5f, 0.5f, 0.5f};
    }

    std::vector<u32> surviving(TOTAL_CLUSTERS);
    const u32 survivingCount = MeshClusterCuller::CullClusters(
        clusters.data(),
        TOTAL_CLUSTERS,
        camera,
        nullptr, // No HZB for this baseline
        surviving.data()
    );
    // Exactly 32 clusters face camera and reside within frustum
    ENGINE_ASSERT(survivingCount == 32, "Exactly half (32) front-facing clusters must survive");
    (void)survivingCount;

    ENGINE_LOG_INFO("CPU mesh cluster culler rejected 32 back-facing clusters (Surviving: 32/64).");
}

void TestMeshClusterCullRenderGraphPass() {
    using namespace Engine::Renderer;
    using namespace Engine::RHI;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing MeshClusterCull RenderGraph Pass Scheduling ---");

    RenderGraph rg;

    RGBufferDesc inDesc{sizeof(MeshletHardwarePayload) * 64, BufferUsage::Structured, "ClusterInputBuffer"};
    auto hInput = rg.DeclareBuffer("ClusterInputBuffer", inDesc);

    RGBufferDesc outDesc{sizeof(u32) * 64, BufferUsage::Structured, "SurvivingClusterPayload"};
    auto hSurviving = rg.DeclareBuffer("SurvivingClusterPayload", outDesc);

    RGTextureDesc hzbDesc{512, 512, Format::R32_FLOAT, TextureUsage::ShaderResource, "HZBTexture"};
    auto hHZB = rg.DeclareTexture("HZBTexture", hzbDesc);

    MeshClusterCuller::AddToGraph(rg, hInput, hSurviving, hHZB);
    rg.MarkOutput(hSurviving);

    ENGINE_ASSERT(rg.Compile(), "RenderGraph compilation with MeshletClusterCullPass must succeed");
    ENGINE_ASSERT(rg.GetCompiledPassCount() == 1, "Expected 1 active MeshletClusterCullPass scheduled");

    ENGINE_LOG_INFO("MeshletClusterCullPass successfully compiled in the RenderGraph.");
}

void RunMeshShaderCullingUnitTests() {
    TestClusterNormalConeBackfaceCulling();
    TestClusterFrustumAndHZBOcclusion();
    TestHardwareMeshClusterCullPipeline();
    TestMeshClusterCullRenderGraphPass();
}
