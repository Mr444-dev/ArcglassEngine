#include "engine/rhi/src/d3d12/D3D12Sync.h"

namespace Engine::RHI::D3D12 {

D3D12Fence::D3D12Fence(ID3D12Device* device, Core::u64 initialValue) {
    ENGINE_ASSERT(device != nullptr, "Device cannot be null for fence creation");
    DX_CHECK(device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_eventHandle = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    ENGINE_ASSERT(m_eventHandle != nullptr, "Failed to create Win32 event for D3D12 fence");
}

D3D12Fence::~D3D12Fence() {
    if (m_eventHandle) {
        CloseHandle(m_eventHandle);
        m_eventHandle = nullptr;
    }
}

void D3D12Fence::Signal(Core::u64 value) {
    DX_CHECK(m_fence->Signal(value));
}

void D3D12Fence::WaitOnCPU(Core::u64 value) {
    if (m_fence->GetCompletedValue() < value) {
        DX_CHECK(m_fence->SetEventOnCompletion(value, m_eventHandle));
        WaitForSingleObject(m_eventHandle, INFINITE);
    }
}

Core::u64 D3D12Fence::GetCompletedValue() const noexcept {
    return m_fence->GetCompletedValue();
}

} // namespace Engine::RHI::D3D12
