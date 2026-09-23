#pragma once

#include "engine/rhi/RHISwapChain.h"
#include "engine/rhi/src/d3d12/D3D12Common.h"
#include "engine/rhi/src/d3d12/D3D12Texture.h"
#include <array>
#include <memory>

namespace Engine::RHI::D3D12 {

class D3D12SwapChain final : public RHISwapChain {
public:
    D3D12SwapChain(
        IDXGIFactory4* factory,
        ID3D12CommandQueue* queue,
        ID3D12Device* device,
        HWND hwnd,
        Core::u32 width,
        Core::u32 height
    );
    ~D3D12SwapChain() override;

    void Present(bool vsync = true) override;
    void Resize(Core::u32 width, Core::u32 height) override;

    [[nodiscard]] RHITexture* GetCurrentBackBuffer() const noexcept override;
    [[nodiscard]] Core::u32 GetCurrentBackBufferIndex() const noexcept override { return m_currentBufferIndex; }
    [[nodiscard]] Core::u32 GetWidth() const noexcept override { return m_width; }
    [[nodiscard]] Core::u32 GetHeight() const noexcept override { return m_height; }

private:
    void CreateRenderTargets(ID3D12Device* device);

    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    std::array<std::unique_ptr<D3D12Texture>, BUFFER_COUNT> m_backBuffers;
    Core::u32 m_currentBufferIndex{0};
    Core::u32 m_width{0};
    Core::u32 m_height{0};
    Core::u32 m_rtvDescriptorSize{0};
};

} // namespace Engine::RHI::D3D12
