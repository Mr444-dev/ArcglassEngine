#pragma once

#include "engine/rhi/RHIBuffer.h"
#include "engine/rhi/src/d3d12/D3D12Common.h"

namespace Engine::RHI::D3D12 {

class D3D12Buffer final : public RHIBuffer {
public:
    D3D12Buffer(ID3D12Device* device, const BufferDesc& desc);
    ~D3D12Buffer() override;

    [[nodiscard]] Core::u64 GetSizeInBytes() const noexcept override { return m_desc.sizeInBytes; }
    [[nodiscard]] BufferUsage GetUsage() const noexcept override { return m_desc.usage; }
    [[nodiscard]] ResourceState GetCurrentState() const noexcept override { return m_currentState; }
    void SetCurrentState(ResourceState state) noexcept override { m_currentState = state; }

    [[nodiscard]] void* Map() override;
    void Unmap() override;

    [[nodiscard]] void* GetNativeResource() const noexcept override { return m_resource.Get(); }
    [[nodiscard]] Core::u64 GetGPUVirtualAddress() const noexcept override;
    [[nodiscard]] Core::u32 GetBindlessIndex() const noexcept override { return m_bindlessIndex; }

private:
    BufferDesc m_desc;
    ResourceState m_currentState{ResourceState::Common};
    ComPtr<ID3D12Resource> m_resource;
    void* m_mappedData{nullptr};
    Core::u32 m_bindlessIndex{0xFFFFFFFF};
};

} // namespace Engine::RHI::D3D12
