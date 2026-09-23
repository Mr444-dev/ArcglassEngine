#include "engine/renderer/MeshShaderCulling.h"
#include <cmath>
#include <algorithm>

namespace Engine::Renderer {

bool MeshClusterCuller::IsConeBackfacing(const ClusterNormalCone& cone, const Core::Vec3& cameraPos) noexcept {
    const Core::Vec3 toApex = cone.apex - cameraPos;
    const float distSq = toApex.Dot(toApex);
    if (distSq < 1e-6f) {
        return false;
    }

    const float invDist = 1.0f / std::sqrt(distSq);
    const Core::Vec3 viewDir = toApex * invDist;

    const float dotAxis = viewDir.Dot(cone.axis);
    const float cosAngle = std::clamp(cone.cutoffAngleCosine, -1.0f, 1.0f);
    const float sinAngle = std::sqrt(std::max(0.0f, 1.0f - cosAngle * cosAngle));

    return dotAxis >= sinAngle;
}

bool MeshClusterCuller::IsFrustumCulled(const Core::AABB& aabb, const Frustum& frustum) noexcept {
    return !frustum.IntersectsAABB(aabb);
}

bool MeshClusterCuller::IsOccluded(
    const Core::AABB& aabb,
    const Camera& camera,
    const HierarchicalDepthBuffer* hzb
) noexcept {
    if (!hzb) return false;
    return hzb->IsAABBOccluded(aabb, camera);
}

Core::u32 MeshClusterCuller::CullClusters(
    const MeshletHardwarePayload* clusters,
    Core::u32 clusterCount,
    const Camera& camera,
    const HierarchicalDepthBuffer* hzb,
    Core::u32* outSurvivingIndices
) noexcept {
    if (!clusters || clusterCount == 0 || !outSurvivingIndices) return 0;

    const auto& frustum = camera.GetFrustum();
    const Core::Vec3& camPos = camera.GetPosition();

    Core::u32 survivingCount = 0;

    for (Core::u32 i = 0; i < clusterCount; ++i) {
        const auto& c = clusters[i];

        // 1. Frustum Culling
        if (IsFrustumCulled(c.cluster.boundingBox, frustum)) {
            continue;
        }

        // 2. Normal Cone Backface Culling
        if (IsConeBackfacing(c.normalCone, camPos)) {
            continue;
        }

        // 3. Hierarchical Z-Buffer (HZB) Occlusion Culling
        if (IsOccluded(c.cluster.boundingBox, camera, hzb)) {
            continue;
        }

        // Cluster survives all hardware culling stages -> append to payload
        outSurvivingIndices[survivingCount++] = i;
    }

    return survivingCount;
}

void MeshClusterCuller::AddToGraph(
    RenderGraph& rg,
    RGBufferHandle clusterInputBuffer,
    RGBufferHandle survivingClusterPayload,
    RGTextureHandle hzbTexture
) {
    rg.AddPass("MeshletClusterCullPass",
        [&](RGPassBuilder& builder) {
            builder.ReadBuffer(clusterInputBuffer, RHI::ResourceState::ShaderResource);
            builder.ReadTexture(hzbTexture, RHI::ResourceState::ShaderResource);
            builder.WriteBuffer(survivingClusterPayload, RHI::ResourceState::UnorderedAccess);
        },
        [clusterInputBuffer](RHI::RHICommandList& cmdList, const RenderGraph& graph) {
            const auto* inBuffer = graph.GetPhysicalBuffer(clusterInputBuffer);
            if (inBuffer) {
                const Core::u32 totalClusters = static_cast<Core::u32>(inBuffer->GetSizeInBytes() / sizeof(MeshletHardwarePayload));
                const Core::u32 threadgroups = (totalClusters + 31) / 32;
                if (threadgroups > 0) {
                    cmdList.DispatchMesh(threadgroups, 1, 1);
                }
            }
        }
    );
}

} // namespace Engine::Renderer
