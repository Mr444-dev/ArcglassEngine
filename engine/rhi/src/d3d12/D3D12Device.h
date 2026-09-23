#pragma once

#include "engine/rhi/RHIDevice.h"
#include "engine/rhi/src/d3d12/D3D12Common.h"
#include "engine/rhi/src/d3d12/D3D12DescriptorHeap.h"
#include "engine/rhi/src/d3d12/D3D12Sync.h"
#include <memory>

namespace Engine::RHI::D3D12 {

class D3D12Device final : public RHIDevice {
public:
    D3D12Device() = default;
    ~D3D12Device() override { Shutdown(); }

    bool Initialize(void* nativeWindowHandle, Core::u32 width, Core::u32 height) override;
    void Shutdown() override;

    [[nodiscard]] std::unique_ptr<RHIBuffer> CreateBuffer(const BufferDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHITexture> CreateTexture(const TextureDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHICommandList> CreateCommandList() override;
    [[nodiscard]] std::unique_ptr<RHIFence> CreateFence() override;
    [[nodiscard]] std::unique_ptr<RHISwapChain> CreateSwapChain(void* nativeWindowHandle, Core::u32 width, Core::u32 height) override;

    void ExecuteCommandList(RHICommandList* commandList) override;
    void ExecuteCommandLists(Core::u32 count, RHICommandList* const* commandLists) override;

    void WaitForGPU() override;

    [[nodiscard]] RHIDescriptorHeap* GetDescriptorHeap() const noexcept override { return m_bindlessHeap.get(); }
    [[nodiscard]] void* GetNativeDevice() const noexcept override { return m_device.Get(); }
    [[nodiscard]] ID3D12CommandQueue* GetDirectQueue() const noexcept { return m_directQueue.Get(); }
    [[nodiscard]] IDXGIFactory4* GetFactory() const noexcept { return m_factory.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE AllocateRTV();
    D3D12_CPU_DESCRIPTOR_HANDLE AllocateDSV();

private:
    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<IDXGIAdapter1> m_adapter;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_directQueue;

    std::unique_ptr<D3D12DescriptorHeap> m_bindlessHeap;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    Core::u32 m_rtvDescriptorSize{0};
    Core::u32 m_dsvDescriptorSize{0};
    Core::u32 m_nextRtvIndex{0};
    Core::u32 m_nextDsvIndex{0};

    std::unique_ptr<D3D12Fence> m_frameFence;
    Core::u64 m_fenceValue{0};
};

} // namespace Engine::RHI::D3D12
