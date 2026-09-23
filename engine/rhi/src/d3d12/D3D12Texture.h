#pragma once

#include "engine/rhi/RHITexture.h"
#include "engine/rhi/src/d3d12/D3D12Common.h"

namespace Engine::RHI::D3D12 {

class D3D12Texture final : public RHITexture {
public:
    // Create new allocated texture
    D3D12Texture(ID3D12Device* device, const TextureDesc& desc);

    // Wrap existing resource (e.g., SwapChain back buffer)
    D3D12Texture(const TextureDesc& desc, ComPtr<ID3D12Resource> existingResource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);

    ~D3D12Texture() override;

    [[nodiscard]] Core::u32 GetWidth() const noexcept override { return m_desc.width; }
    [[nodiscard]] Core::u32 GetHeight() const noexcept override { return m_desc.height; }
    [[nodiscard]] Core::u32 GetDepth() const noexcept override { return m_desc.depth; }
    [[nodiscard]] Core::u32 GetMipLevels() const noexcept override { return m_desc.mipLevels; }
    [[nodiscard]] Format GetFormat() const noexcept override { return m_desc.format; }
    [[nodiscard]] TextureUsage GetUsage() const noexcept override { return m_desc.usage; }

    [[nodiscard]] ResourceState GetCurrentState() const noexcept override { return m_currentState; }
    void SetCurrentState(ResourceState state) noexcept override { m_currentState = state; }

    [[nodiscard]] void* GetNativeResource() const noexcept override { return m_resource.Get(); }
    [[nodiscard]] Core::u32 GetBindlessIndex() const noexcept override { return m_bindlessIndex; }
    [[nodiscard]] size_t GetRTVDescriptorHandle() const noexcept override { return m_rtvHandle.ptr; }
    [[nodiscard]] size_t GetDSVDescriptorHandle() const noexcept override { return m_dsvHandle.ptr; }

private:
    TextureDesc m_desc;
    ResourceState m_currentState{ResourceState::Common};
    ComPtr<ID3D12Resource> m_resource;
    Core::u32 m_bindlessIndex{0xFFFFFFFF};
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle{0};
    D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHandle{0};
    bool m_ownsResource{true};
};

} // namespace Engine::RHI::D3D12
