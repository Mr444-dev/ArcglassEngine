#include "engine/rhi/src/d3d12/D3D12Texture.h"
#include "engine/rhi/src/d3d12/D3D12Device.h"

namespace Engine::RHI::D3D12 {

D3D12Texture::D3D12Texture(ID3D12Device* device, const TextureDesc& desc)
    : m_desc(desc)
    , m_ownsResource(true)
{
    ENGINE_ASSERT(device != nullptr, "Device cannot be null");

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC rDesc{};
    rDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rDesc.Alignment = 0;
    rDesc.Width = desc.width;
    rDesc.Height = desc.height;
    rDesc.DepthOrArraySize = static_cast<UINT16>(desc.depth);
    rDesc.MipLevels = static_cast<UINT16>(desc.mipLevels);
    rDesc.Format = ConvertFormat(desc.format);
    rDesc.SampleDesc.Count = 1;
    rDesc.SampleDesc.Quality = 0;
    rDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    rDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_RESOURCE_STATES initialD3DState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_CLEAR_VALUE clearValue{};
    D3D12_CLEAR_VALUE* pClearValue = nullptr;

    if (desc.usage & TextureUsage::RenderTarget) {
        rDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        clearValue.Format = rDesc.Format;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;
        pClearValue = &clearValue;
    }

    if (desc.usage & TextureUsage::DepthStencil) {
        rDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        initialD3DState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        m_currentState = ResourceState::DepthWrite;
        clearValue.Format = rDesc.Format;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;
        pClearValue = &clearValue;
    }

    if (desc.usage & TextureUsage::UnorderedAccess) {
        rDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    DX_CHECK(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &rDesc,
        initialD3DState,
        pClearValue,
        IID_PPV_ARGS(&m_resource)
    ));

    if (desc.usage & TextureUsage::RenderTarget) {
        m_rtvHandle = static_cast<D3D12Device&>(RHIDevice::Get()).AllocateRTV();
        if (m_rtvHandle.ptr != 0) {
            device->CreateRenderTargetView(m_resource.Get(), nullptr, m_rtvHandle);
        }
    }

    if (desc.usage & TextureUsage::DepthStencil) {
        m_dsvHandle = static_cast<D3D12Device&>(RHIDevice::Get()).AllocateDSV();
        if (m_dsvHandle.ptr != 0) {
            device->CreateDepthStencilView(m_resource.Get(), nullptr, m_dsvHandle);
        }
    }

    if (!desc.debugName.empty()) {
        std::wstring wName(desc.debugName.begin(), desc.debugName.end());
        m_resource->SetName(wName.c_str());
    }
}

D3D12Texture::D3D12Texture(const TextureDesc& desc, ComPtr<ID3D12Resource> existingResource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
    : m_desc(desc)
    , m_currentState(ResourceState::Common)
    , m_resource(std::move(existingResource))
    , m_rtvHandle(rtvHandle)
    , m_ownsResource(false)
{
}

D3D12Texture::~D3D12Texture() {
    m_resource.Reset();
}

} // namespace Engine::RHI::D3D12
