#include "engine/rhi/src/d3d12/D3D12Buffer.h"

namespace Engine::RHI::D3D12 {

D3D12Buffer::D3D12Buffer(ID3D12Device* device, const BufferDesc& desc)
    : m_desc(desc)
{
    ENGINE_ASSERT(device != nullptr, "Device cannot be null");
    ENGINE_ASSERT(desc.sizeInBytes > 0, "Buffer size must be greater than zero");

    D3D12_HEAP_PROPERTIES heapProps{};
    D3D12_RESOURCE_STATES initialD3DState = D3D12_RESOURCE_STATE_COMMON;

    if (desc.usage & BufferUsage::Upload) {
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        initialD3DState = D3D12_RESOURCE_STATE_GENERIC_READ;
        m_currentState = ResourceState::Common;
    } else {
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        initialD3DState = D3D12_RESOURCE_STATE_COMMON;
        m_currentState = ResourceState::Common;
    }

    D3D12_RESOURCE_DESC rDesc{};
    rDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rDesc.Alignment = 0;
    rDesc.Width = desc.sizeInBytes;
    rDesc.Height = 1;
    rDesc.DepthOrArraySize = 1;
    rDesc.MipLevels = 1;
    rDesc.Format = DXGI_FORMAT_UNKNOWN;
    rDesc.SampleDesc.Count = 1;
    rDesc.SampleDesc.Quality = 0;
    rDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    DX_CHECK(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &rDesc,
        initialD3DState,
        nullptr,
        IID_PPV_ARGS(&m_resource)
    ));

    if (!desc.debugName.empty()) {
        std::wstring wName(desc.debugName.begin(), desc.debugName.end());
        m_resource->SetName(wName.c_str());
    }
}

D3D12Buffer::~D3D12Buffer() {
    Unmap();
    m_resource.Reset();
}

void* D3D12Buffer::Map() {
    if (!m_mappedData && (m_desc.usage & BufferUsage::Upload)) {
        D3D12_RANGE readRange{0, 0}; // We do not intend to read on the CPU
        DX_CHECK(m_resource->Map(0, &readRange, &m_mappedData));
    }
    return m_mappedData;
}

void D3D12Buffer::Unmap() {
    if (m_mappedData) {
        m_resource->Unmap(0, nullptr);
        m_mappedData = nullptr;
    }
}

Core::u64 D3D12Buffer::GetGPUVirtualAddress() const noexcept {
    return m_resource ? m_resource->GetGPUVirtualAddress() : 0;
}

} // namespace Engine::RHI::D3D12
