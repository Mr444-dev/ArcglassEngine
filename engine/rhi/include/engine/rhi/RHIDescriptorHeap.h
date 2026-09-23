#pragma once

#include "engine/core/BaseTypes.h"
#include <vector>
#include <mutex>

namespace Engine::RHI {

class RHIDescriptorHeap {
public:
    static constexpr Core::u32 MAX_BINDLESS_DESCRIPTORS = 500000;
    static constexpr Core::u32 INVALID_DESCRIPTOR_INDEX = 0xFFFFFFFF;

    RHIDescriptorHeap() = default;
    virtual ~RHIDescriptorHeap() = default;

    virtual bool Initialize(void* nativeDevice, Core::u32 capacity = MAX_BINDLESS_DESCRIPTORS) = 0;
    virtual void Shutdown() = 0;

    // Allocate an index in the bindless heap
    virtual Core::u32 AllocateIndex() = 0;
    virtual void FreeIndex(Core::u32 index) = 0;

    [[nodiscard]] virtual void* GetNativeHeap() const noexcept = 0;
    [[nodiscard]] virtual Core::u64 GetGPUDescriptorHandleForIndex(Core::u32 index) const noexcept = 0;
    [[nodiscard]] virtual size_t GetCPUDescriptorHandleForIndex(Core::u32 index) const noexcept = 0;

    [[nodiscard]] Core::u32 GetAllocatedCount() const noexcept { return m_allocatedCount; }
    [[nodiscard]] Core::u32 GetCapacity() const noexcept { return m_capacity; }

protected:
    Core::u32 m_capacity{0};
    Core::u32 m_allocatedCount{0};
    std::vector<Core::u32> m_freeIndices;
    mutable std::mutex m_allocationMutex;
};

} // namespace Engine::RHI
