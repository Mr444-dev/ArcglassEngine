#pragma once

#include "engine/rhi/RHISync.h"
#include "engine/rhi/src/d3d12/D3D12Common.h"

namespace Engine::RHI::D3D12 {

class D3D12Fence final : public RHIFence {
public:
    explicit D3D12Fence(ID3D12Device* device, Core::u64 initialValue = 0);
    ~D3D12Fence() override;

    void Signal(Core::u64 value) override;
    void WaitOnCPU(Core::u64 value) override;
    [[nodiscard]] Core::u64 GetCompletedValue() const noexcept override;
    [[nodiscard]] void* GetNativeFence() const noexcept override { return m_fence.Get(); }

private:
    ComPtr<ID3D12Fence> m_fence;
    HANDLE m_eventHandle{nullptr};
};

} // namespace Engine::RHI::D3D12
