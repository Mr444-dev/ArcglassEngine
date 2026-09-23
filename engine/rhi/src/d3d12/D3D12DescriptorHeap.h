#pragma once

#include "engine/rhi/RHIDescriptorHeap.h"
#include "engine/rhi/src/d3d12/D3D12Common.h"

namespace Engine::RHI::D3D12 {

class D3D12DescriptorHeap final : public RHIDescriptorHeap {
public:
    D3D12DescriptorHeap() = default;
    ~D3D12DescriptorHeap() override { Shutdown(); }

    bool Initialize(void* nativeDevice, Core::u32 capacity = MAX_BINDLESS_DESCRIPTORS) override;
    void Shutdown() override;

    Core::u32 AllocateIndex() override;
    void FreeIndex(Core::u32 index) override;

    [[nodiscard]] void* GetNativeHeap() const noexcept override { return m_heap.Get(); }
    [[nodiscard]] Core::u64 GetGPUDescriptorHandleForIndex(Core::u32 index) const noexcept override;
    [[nodiscard]] size_t GetCPUDescriptorHandleForIndex(Core::u32 index) const noexcept override;

    [[nodiscard]] Core::u32 GetDescriptorIncrementSize() const noexcept { return m_incrementSize; }

private:
    ComPtr<ID3D12DescriptorHeap> m_heap;
    Core::u32 m_incrementSize{0};
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuStart{0};
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuStart{0};
};

} // namespace Engine::RHI::D3D12
