#include "engine/rhi/src/d3d12/D3D12SwapChain.h"

namespace Engine::RHI::D3D12 {

D3D12SwapChain::D3D12SwapChain(
    IDXGIFactory4* factory,
    ID3D12CommandQueue* queue,
    ID3D12Device* device,
    HWND hwnd,
    Core::u32 width,
    Core::u32 height
)
    : m_width(width)
    , m_height(height)
{
    ENGINE_ASSERT(factory && queue && device && hwnd, "Null parameter passed to D3D12SwapChain");

    // 1. Create RTV Descriptor Heap for backbuffers
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = BUFFER_COUNT;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DX_CHECK(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
    m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // 2. Configure DXGI SwapChain Desc
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = BUFFER_COUNT;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Modern flip model
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    ComPtr<IDXGISwapChain1> baseSwapChain;
    DX_CHECK(factory->CreateSwapChainForHwnd(
        queue,
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &baseSwapChain
    ));

    DX_CHECK(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    DX_CHECK(baseSwapChain.As(&m_swapChain));

    m_currentBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    CreateRenderTargets(device);

    ENGINE_LOG_INFO("D3D12 SwapChain initialized ({}x{}, Triple-Buffered Flip Discard).", m_width, m_height);
}

D3D12SwapChain::~D3D12SwapChain() {
    for (auto& buffer : m_backBuffers) {
        buffer.reset();
    }
    m_rtvHeap.Reset();
    m_swapChain.Reset();
}

void D3D12SwapChain::CreateRenderTargets(ID3D12Device* device) {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (Core::u32 i = 0; i < BUFFER_COUNT; ++i) {
        ComPtr<ID3D12Resource> backBufferResource;
        DX_CHECK(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBufferResource)));

        device->CreateRenderTargetView(backBufferResource.Get(), nullptr, rtvHandle);

        TextureDesc tDesc{};
        tDesc.width = m_width;
        tDesc.height = m_height;
        tDesc.format = Format::RGBA8_UNORM;
        tDesc.usage = TextureUsage::RenderTarget;
        tDesc.debugName = "SwapChain_BackBuffer_" + std::to_string(i);

        m_backBuffers[i] = std::make_unique<D3D12Texture>(tDesc, backBufferResource, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }
}

void D3D12SwapChain::Present(bool vsync) {
    HRESULT hr = m_swapChain->Present(vsync ? 1 : 0, 0);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            ENGINE_LOG_FATAL("GPU Device Lost / Removed during Present.");
        }
        DX_CHECK(hr);
    }
    m_currentBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void D3D12SwapChain::Resize(Core::u32 width, Core::u32 height) {
    if (width == 0 || height == 0 || (width == m_width && height == m_height)) return;

    m_width = width;
    m_height = height;

    for (auto& buffer : m_backBuffers) {
        buffer.reset();
    }

    DX_CHECK(m_swapChain->ResizeBuffers(
        BUFFER_COUNT,
        m_width,
        m_height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        0
    ));

    m_currentBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    // Device retrieval
    ComPtr<ID3D12Device> device;
    DX_CHECK(m_swapChain->GetDevice(IID_PPV_ARGS(&device)));
    CreateRenderTargets(device.Get());

    ENGINE_LOG_INFO("D3D12 SwapChain resized to {}x{}.", m_width, m_height);
}

RHITexture* D3D12SwapChain::GetCurrentBackBuffer() const noexcept {
    return m_backBuffers[m_currentBufferIndex].get();
}

} // namespace Engine::RHI::D3D12
