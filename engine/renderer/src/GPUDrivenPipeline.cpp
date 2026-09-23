#include "engine/renderer/GPUDrivenPipeline.h"
#include <cmath>

namespace Engine::Renderer {

GPUDrivenPipeline::GPUDrivenPipeline() = default;

void GPUDrivenPipeline::SetInstances(std::vector<GPUInstanceData> instances) {
    m_instances = std::move(instances);
}

void GPUDrivenPipeline::AddInstance(const GPUInstanceData& instance) {
    m_instances.push_back(instance);
}

void GPUDrivenPipeline::ClearInstances() {
    m_instances.clear();
    m_visibleCount = 0;
}

void GPUDrivenPipeline::CullAndGenerateCommands(
    const Camera& camera,
    float lod1Distance,
    float lod2Distance,
    std::vector<IndirectDrawCommand>& outCommands
) {
    outCommands.clear();
    outCommands.reserve(m_instances.size());
    m_visibleCount = 0;

    const auto& frustum = camera.GetFrustum();
    const Core::Vec3 camPos = camera.GetPosition();

    for (size_t i = 0; i < m_instances.size(); ++i) {
        const auto& instance = m_instances[i];

        // 1. Transform AABB to World Space
        const Core::Vec3 center = (instance.aabbMin + instance.aabbMax) * 0.5f;
        const Core::Vec3 extents = (instance.aabbMax - instance.aabbMin) * 0.5f;

        // Position from world matrix row 3 (elements m[12], m[13], m[14])
        const Core::Vec3 translation{
            instance.worldMatrix.m[12],
            instance.worldMatrix.m[13],
            instance.worldMatrix.m[14]
        };
        const Core::Vec3 worldCenter = center + translation;

        const Core::AABB worldAABB{
            worldCenter - extents,
            worldCenter + extents
        };

        // 2. Frustum Culling
        if (!frustum.IntersectsAABB(worldAABB)) {
            continue; // Culled!
        }

        // 3. Distance-based LOD selection
        const float dist = (worldCenter - camPos).Length();
        Core::u32 indexCount = instance.indexCount;
        if (dist > lod2Distance) {
            indexCount = instance.indexCount / 4; // LOD 2
        } else if (dist > lod1Distance) {
            indexCount = instance.indexCount / 2; // LOD 1
        }

        // 4. Emit Indirect Command
        IndirectDrawCommand cmd{};
        cmd.indexCountPerInstance = indexCount;
        cmd.instanceCount = 1;
        cmd.startIndexLocation = instance.startIndexLocation;
        cmd.baseVertexLocation = instance.baseVertexLocation;
        cmd.startInstanceLocation = static_cast<Core::u32>(i);
        cmd.materialID = instance.materialID;

        outCommands.push_back(cmd);
        m_visibleCount++;
    }
}

} // namespace Engine::Renderer
