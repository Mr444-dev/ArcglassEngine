#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include "engine/renderer/Camera.h"
#include <vector>

namespace Engine::Renderer {

struct alignas(16) GPUInstanceData {
    Core::Mat4x4 worldMatrix;
    Core::Vec3 aabbMin{-1.0f, -1.0f, -1.0f};
    float pad0{0.0f};
    Core::Vec3 aabbMax{1.0f, 1.0f, 1.0f};
    float pad1{0.0f};
    Core::u32 meshID{0};
    Core::u32 materialID{0};
    Core::u32 indexCount{0};
    Core::u32 startIndexLocation{0};
    Core::i32 baseVertexLocation{0};
    Core::u32 pad2[3]{0, 0, 0};
};

struct alignas(16) IndirectDrawCommand {
    Core::u32 indexCountPerInstance{0};
    Core::u32 instanceCount{1};
    Core::u32 startIndexLocation{0};
    Core::i32 baseVertexLocation{0};
    Core::u32 startInstanceLocation{0};
    Core::u32 materialID{0};
    Core::u32 pad[2]{0, 0};
};

class GPUDrivenPipeline {
public:
    GPUDrivenPipeline();
    ~GPUDrivenPipeline() = default;

    void SetInstances(std::vector<GPUInstanceData> instances);
    void AddInstance(const GPUInstanceData& instance);
    void ClearInstances();

    void CullAndGenerateCommands(
        const Camera& camera,
        float lod1Distance,
        float lod2Distance,
        std::vector<IndirectDrawCommand>& outCommands
    );

    [[nodiscard]] size_t GetTotalInstanceCount() const noexcept { return m_instances.size(); }
    [[nodiscard]] size_t GetVisibleInstanceCount() const noexcept { return m_visibleCount; }

private:
    std::vector<GPUInstanceData> m_instances;
    size_t m_visibleCount{0};
};

} // namespace Engine::Renderer
