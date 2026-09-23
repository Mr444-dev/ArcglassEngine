#include "engine/renderer/MaterialSystem.h"
#include "engine/core/Assert.h"
#include "engine/core/Log.h"
#include <cstring>

namespace Engine::Renderer {

bool MaterialSystem::Initialize() {
    auto& device = RHI::RHIDevice::Get();

    RHI::BufferDesc bDesc{};
    bDesc.sizeInBytes = sizeof(GPUMaterialData) * MAX_MATERIALS;
    bDesc.usage = RHI::BufferUsage::Structured | RHI::BufferUsage::Upload;
    bDesc.debugName = "GPUMaterialTable";

    m_gpuBuffer = device.CreateBuffer(bDesc);
    if (!m_gpuBuffer) {
        ENGINE_LOG_ERROR("Failed to create GPU Material Buffer");
        return false;
    }

    m_materials.reserve(256);

    // Register default fallback material at index 0
    GPUMaterialData defaultMat{};
    defaultMat.baseColorFactor[0] = 0.8f;
    defaultMat.baseColorFactor[1] = 0.8f;
    defaultMat.baseColorFactor[2] = 0.8f;
    defaultMat.roughnessFactor = 0.5f;
    defaultMat.metallicFactor = 0.0f;
    (void)RegisterMaterial(defaultMat);

    FlushToGPU();
    ENGINE_LOG_INFO("MaterialSystem initialized (Capacity: {} materials).", MAX_MATERIALS);
    return true;
}

void MaterialSystem::Shutdown() {
    m_gpuBuffer.reset();
    m_materials.clear();
    m_isDirty = false;
}

Core::u32 MaterialSystem::RegisterMaterial(const GPUMaterialData& material) {
    ENGINE_ASSERT(m_materials.size() < MAX_MATERIALS, "Exceeded maximum material count");
    const Core::u32 id = static_cast<Core::u32>(m_materials.size());
    m_materials.push_back(material);
    m_isDirty = true;
    return id;
}

void MaterialSystem::UpdateMaterial(Core::u32 materialId, const GPUMaterialData& material) {
    ENGINE_ASSERT(materialId < m_materials.size(), "Material ID out of range");
    m_materials[materialId] = material;
    m_isDirty = true;
}

void MaterialSystem::FlushToGPU() {
    if (!m_isDirty || !m_gpuBuffer || m_materials.empty()) return;

    void* mapped = m_gpuBuffer->Map();
    if (mapped) {
        std::memcpy(mapped, m_materials.data(), sizeof(GPUMaterialData) * m_materials.size());
        m_gpuBuffer->Unmap();
    }
    m_isDirty = false;
}

} // namespace Engine::Renderer
