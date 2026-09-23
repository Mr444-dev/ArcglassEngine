#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/Assert.h"
#include <new>
#include <utility>

namespace Engine::Core {

// Virtual Memory OS Page Wrapper
class VirtualMemory {
public:
    static void* Reserve(size_t sizeInBytes);
    static bool Commit(void* address, size_t sizeInBytes);
    static void Decommit(void* address, size_t sizeInBytes);
    static void Release(void* address, size_t sizeInBytes);
    static size_t GetPageSize();
};

// Fast Linear/Arena Allocator
class LinearAllocator {
public:
    LinearAllocator() = default;
    explicit LinearAllocator(size_t capacityInBytes);
    ~LinearAllocator();

    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    LinearAllocator(LinearAllocator&& other) noexcept;
    LinearAllocator& operator=(LinearAllocator&& other) noexcept;

    void Initialize(size_t capacityInBytes);
    void Shutdown();

    [[nodiscard]] void* Allocate(size_t sizeInBytes, size_t alignment = alignof(std::max_align_t));

    template <typename T, typename... Args>
    [[nodiscard]] T* Create(Args&&... args) {
        void* memory = Allocate(sizeof(T), alignof(T));
        return new (memory) T(std::forward<Args>(args)...);
    }

    void Reset();

    [[nodiscard]] size_t GetAllocatedBytes() const noexcept { return m_offset; }
    [[nodiscard]] size_t GetCapacityBytes() const noexcept { return m_capacity; }

private:
    u8* m_memory{nullptr};
    size_t m_capacity{0};
    size_t m_offset{0};
    bool m_ownsMemory{false};
};

// Double-Buffered Frame Allocator
class FrameAllocator {
public:
    explicit FrameAllocator(size_t capacityPerFrameBytes);
    ~FrameAllocator() = default;

    void BeginFrame();

    [[nodiscard]] void* Allocate(size_t sizeInBytes, size_t alignment = alignof(std::max_align_t));

    template <typename T, typename... Args>
    [[nodiscard]] T* Create(Args&&... args) {
        return m_arenas[m_currentFrameIndex].Create<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]] size_t GetActiveFrameAllocatedBytes() const noexcept {
        return m_arenas[m_currentFrameIndex].GetAllocatedBytes();
    }

private:
    LinearAllocator m_arenas[2];
    u32 m_currentFrameIndex{0};
};

// Fixed-Block Pool Allocator (O(1) allocation/deallocation without fragmentation)
class PoolAllocator {
public:
    PoolAllocator() = default;
    PoolAllocator(size_t elementSize, size_t elementAlignment, size_t capacity);
    ~PoolAllocator();

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    void Initialize(size_t elementSize, size_t elementAlignment, size_t capacity);
    void Shutdown();

    [[nodiscard]] void* Allocate();
    void Deallocate(void* ptr);

    [[nodiscard]] size_t GetAllocatedCount() const noexcept { return m_allocatedCount; }
    [[nodiscard]] size_t GetCapacity() const noexcept { return m_capacity; }

private:
    struct FreeNode {
        FreeNode* next;
    };

    u8* m_memory{nullptr};
    FreeNode* m_freeList{nullptr};
    size_t m_elementSize{0};
    size_t m_elementAlignment{0};
    size_t m_capacity{0};
    size_t m_allocatedCount{0};
};

} // namespace Engine::Core
