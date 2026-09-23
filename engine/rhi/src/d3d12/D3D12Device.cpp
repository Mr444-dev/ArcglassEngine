#include "engine/rhi/src/d3d12/D3D12Device.h"
#include "engine/rhi/src/d3d12/D3D12Buffer.h"
#include "engine/rhi/src/d3d12/D3D12Texture.h"
#include "engine/rhi/src/d3d12/D3D12PipelineState.h"
#include "engine/rhi/src/d3d12/D3D12CommandList.h"
#include "engine/rhi/src/d3d12/D3D12SwapChain.h"

namespace Engine::RHI {

RHIDevice& RHIDevice::Get() {
    static D3D12::D3D12Device instance;
    return instance;
}

} // namespace Engine::RHI

namespace Engine::RHI::D3D12 {

bool D3D12Device::Initialize(void* nativeWindowHandle, Core::u32 width, Core::u32 height) {
    (void)nativeWindowHandle;
    (void)width;
    (void)height;

    Shutdown();

    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG) || !defined(NDEBUG)
    // Enable D3D12 Debug Layer
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        ENGINE_LOG_INFO("D3D12 Debug Layer enabled successfully.");
    }
#endif

    DX_CHECK(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

    // Select high-performance hardware adapter
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue; // Skip WARP software rasterizer
        }

        // Try creating device to verify feature level 12.0 support
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr))) {
            m_adapter = adapter;
            char sDesc[256]{};
            WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, sDesc, sizeof(sDesc), nullptr, nullptr);
            ENGINE_LOG_INFO("Selected Hardware GPU: {} (Dedicated VRAM: {} MB)", sDesc, desc.DedicatedVideoMemory / (1024 * 1024));
            break;
        }
    }

    if (!m_adapter) {
        ENGINE_LOG_WARN("No hardware GPU found; falling back to first enumerated adapter.");
        m_factory->EnumAdapters1(0, &m_adapter);
    }

    DX_CHECK(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));

    // Create Direct Command Queue
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.NodeMask = 0;
    DX_CHECK(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_directQueue)));

    // Initialize Bindless Descriptor Heap
    m_bindlessHeap = std::make_unique<D3D12DescriptorHeap>();
    ENGINE_VERIFY(m_bindlessHeap->Initialize(m_device.Get()), "Failed to initialize bindless descriptor heap");

    // Initialize RTV Descriptor Heap for offscreen render targets
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.NumDescriptors = 512;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DX_CHECK(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtvHeap)));
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_nextRtvIndex = 0;

    // Initialize DSV Descriptor Heap for offscreen depth-stencil targets
    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
    dsvDesc.NumDescriptors = 128;
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DX_CHECK(m_device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&m_dsvHeap)));
    m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_nextDsvIndex = 0;

    // Initialize frame fence
    m_frameFence = std::make_unique<D3D12Fence>(m_device.Get(), 0);
    m_fenceValue = 0;

    ENGINE_LOG_INFO("D3D12Device initialized successfully (RTV Heap: 512 slots, DSV Heap: 128 slots).");
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Device::AllocateRTV() {
    if (!m_rtvHeap || m_nextRtvIndex >= 512) return {0};
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(m_nextRtvIndex++) * m_rtvDescriptorSize;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Device::AllocateDSV() {
    if (!m_dsvHeap || m_nextDsvIndex >= 128) return {0};
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(m_nextDsvIndex++) * m_dsvDescriptorSize;
    return handle;
}

void D3D12Device::Shutdown() {
    WaitForGPU();

    m_bindlessHeap.reset();
    m_rtvHeap.Reset();
    m_dsvHeap.Reset();
    m_frameFence.reset();
    m_directQueue.Reset();
    m_device.Reset();
    m_adapter.Reset();
    m_factory.Reset();
}

std::unique_ptr<RHIBuffer> D3D12Device::CreateBuffer(const BufferDesc& desc) {
    return std::make_unique<D3D12Buffer>(m_device.Get(), desc);
}

std::unique_ptr<RHITexture> D3D12Device::CreateTexture(const TextureDesc& desc) {
    return std::make_unique<D3D12Texture>(m_device.Get(), desc);
}

std::unique_ptr<RHIPipelineState> D3D12Device::CreateGraphicsPipelineState(const GraphicsPipelineDesc& desc) {
    return std::make_unique<D3D12PipelineState>(m_device.Get(), desc);
}

std::unique_ptr<RHICommandList> D3D12Device::CreateCommandList() {
    return std::make_unique<D3D12CommandList>(m_device.Get());
}

std::unique_ptr<RHIFence> D3D12Device::CreateFence() {
    return std::make_unique<D3D12Fence>(m_device.Get());
}

std::unique_ptr<RHISwapChain> D3D12Device::CreateSwapChain(void* nativeWindowHandle, Core::u32 width, Core::u32 height) {
    return std::make_unique<D3D12SwapChain>(
        m_factory.Get(),
        m_directQueue.Get(),
        m_device.Get(),
        static_cast<HWND>(nativeWindowHandle),
        width,
        height
    );
}

void D3D12Device::ExecuteCommandList(RHICommandList* commandList) {
    RHICommandList* lists[1] = { commandList };
    ExecuteCommandLists(1, lists);
}

void D3D12Device::ExecuteCommandLists(Core::u32 count, RHICommandList* const* commandLists) {
    if (count == 0 || !commandLists) return;

    std::vector<ID3D12CommandList*> d3dLists;
    d3dLists.reserve(count);
    for (Core::u32 i = 0; i < count; ++i) {
        d3dLists.push_back(static_cast<ID3D12CommandList*>(commandLists[i]->GetNativeCommandList()));
    }

    m_directQueue->ExecuteCommandLists(count, d3dLists.data());
}

void D3D12Device::WaitForGPU() {
    if (!m_directQueue || !m_frameFence) return;

    m_fenceValue++;
    DX_CHECK(m_directQueue->Signal(static_cast<ID3D12Fence*>(m_frameFence->GetNativeFence()), m_fenceValue));
    m_frameFence->WaitOnCPU(m_fenceValue);
}

} // namespace Engine::RHI::D3D12
