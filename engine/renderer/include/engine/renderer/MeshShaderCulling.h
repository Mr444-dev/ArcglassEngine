#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/renderer/MeshletLODDAG.h"
#include "engine/renderer/HierarchicalDepthBuffer.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/RenderGraph.h"

namespace Engine::Renderer {

// Bounding Normal Cone for cluster backface culling in Task / Amplification Shaders
struct ClusterNormalCone {
    Core::Vec3 apex{0.0f, 0.0f, 0.0f};
    Core::Vec3 axis{0.0f, 1.0f, 0.0f}; // Cluster average normal vector
    float cutoffAngleCosine{0.0f};     // cos(semi-angle) in [-1, 1]
};

struct MeshletHardwarePayload {
    MeshletCluster cluster;
    ClusterNormalCone normalCone;
};

class MeshClusterCuller {
public:
    MeshClusterCuller() = default;

    // Evaluates whether a cluster's bounding normal cone faces entirely away from camera (Zero front-facing triangles)
    [[nodiscard]] static bool IsConeBackfacing(const ClusterNormalCone& cone, const Core::Vec3& cameraPos) noexcept;

    // Evaluates frustum intersection against camera view frustum planes
    [[nodiscard]] static bool IsFrustumCulled(const Core::AABB& aabb, const Frustum& frustum) noexcept;

    // Evaluates occlusion against Hierarchical Depth Buffer (HZB)
    [[nodiscard]] static bool IsOccluded(
        const Core::AABB& aabb,
        const Camera& camera,
        const HierarchicalDepthBuffer* hzb
    ) noexcept;

    // Full GPU micro-culling pipeline: Frustum -> Backface Cone -> HZB Occlusion
    [[nodiscard]] static Core::u32 CullClusters(
        const MeshletHardwarePayload* clusters,
        Core::u32 clusterCount,
        const Camera& camera,
        const HierarchicalDepthBuffer* hzb,
        Core::u32* outSurvivingIndices
    ) noexcept;

    static void AddToGraph(
        RenderGraph& rg,
        RGBufferHandle clusterInputBuffer,
        RGBufferHandle survivingClusterPayload,
        RGTextureHandle hzbTexture
    );
};

} // namespace Engine::Renderer
