#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/rhi/RHIBuffer.h"
#include "engine/rhi/RHIDevice.h"
#include <vector>
#include <memory>

namespace Engine::Renderer {

struct alignas(16) GPUMaterialData {
    Core::u32 albedoTextureIndex{0xFFFFFFFF};
    Core::u32 normalTextureIndex{0xFFFFFFFF};
    Core::u32 metallicRoughnessTextureIndex{0xFFFFFFFF};
    Core::u32 emissiveTextureIndex{0xFFFFFFFF};

    float baseColorFactor[4]{1.0f, 1.0f, 1.0f, 1.0f};

    float roughnessFactor{1.0f};
    float metallicFactor{0.0f};
    float alphaCutoff{0.5f};
    float padding0{0.0f};

    float emissiveFactor[3]{0.0f, 0.0f, 0.0f};
    float padding1{0.0f};
};

class MaterialSystem {
public:
    static constexpr Core::u32 MAX_MATERIALS = 4096;

    MaterialSystem() = default;
    ~MaterialSystem() = default;

    bool Initialize();
    void Shutdown();

    [[nodiscard]] Core::u32 RegisterMaterial(const GPUMaterialData& material);
    void UpdateMaterial(Core::u32 materialId, const GPUMaterialData& material);

    void FlushToGPU();

    [[nodiscard]] RHI::RHIBuffer* GetGPUBuffer() const noexcept { return m_gpuBuffer.get(); }
    [[nodiscard]] Core::u32 GetMaterialCount() const noexcept { return static_cast<Core::u32>(m_materials.size()); }

private:
    std::vector<GPUMaterialData> m_materials;
    std::unique_ptr<RHI::RHIBuffer> m_gpuBuffer;
    bool m_isDirty{false};
};

} // namespace Engine::Renderer
