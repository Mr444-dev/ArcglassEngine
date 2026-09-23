#include "engine/rhi/src/d3d12/D3D12DescriptorHeap.h"

namespace Engine::RHI::D3D12 {

bool D3D12DescriptorHeap::Initialize(void* nativeDevice, Core::u32 capacity) {
    Shutdown();

    auto* device = static_cast<ID3D12Device*>(nativeDevice);
    if (!device) return false;

    m_capacity = capacity;
    m_allocatedCount = 0;

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = m_capacity;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;

    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap));
    if (FAILED(hr)) {
        ENGINE_LOG_FATAL("Failed to create D3D12 shader-visible descriptor heap.");
        return false;
    }

    m_incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_cpuStart = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_gpuStart = m_heap->GetGPUDescriptorHandleForHeapStart();

    // Prepopulate free list
    m_freeIndices.reserve(m_capacity);
    for (Core::u32 i = 0; i < m_capacity; ++i) {
        m_freeIndices.push_back(m_capacity - 1 - i); // LIFO allocation
    }

    ENGINE_LOG_INFO("D3D12 Bindless Descriptor Heap initialized with {} slots ({} KB).", 
        m_capacity, (m_capacity * m_incrementSize) / 1024);
    return true;
}

void D3D12DescriptorHeap::Shutdown() {
    std::lock_guard<std::mutex> lock(m_allocationMutex);
    m_heap.Reset();
    m_freeIndices.clear();
    m_capacity = 0;
    m_allocatedCount = 0;
    m_incrementSize = 0;
    m_cpuStart.ptr = 0;
    m_gpuStart.ptr = 0;
}

Core::u32 D3D12DescriptorHeap::AllocateIndex() {
    std::lock_guard<std::mutex> lock(m_allocationMutex);
    if (m_freeIndices.empty()) {
        ENGINE_ASSERT(false, "Out of bindless descriptors");
        return INVALID_DESCRIPTOR_INDEX;
    }

    const Core::u32 index = m_freeIndices.back();
    m_freeIndices.pop_back();
    m_allocatedCount++;
    return index;
}

void D3D12DescriptorHeap::FreeIndex(Core::u32 index) {
    if (index == INVALID_DESCRIPTOR_INDEX) return;

    std::lock_guard<std::mutex> lock(m_allocationMutex);
    m_freeIndices.push_back(index);
    if (m_allocatedCount > 0) {
        m_allocatedCount--;
    }
}

Core::u64 D3D12DescriptorHeap::GetGPUDescriptorHandleForIndex(Core::u32 index) const noexcept {
    if (index >= m_capacity) return 0;
    return m_gpuStart.ptr + static_cast<Core::u64>(index) * m_incrementSize;
}

size_t D3D12DescriptorHeap::GetCPUDescriptorHandleForIndex(Core::u32 index) const noexcept {
    if (index >= m_capacity) return 0;
    return m_cpuStart.ptr + static_cast<size_t>(index) * m_incrementSize;
}

} // namespace Engine::RHI::D3D12
