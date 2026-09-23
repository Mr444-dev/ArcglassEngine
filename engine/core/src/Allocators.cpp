#include "engine/core/Allocators.h"
#include "engine/core/Profiler.h"
#include <algorithm>
#include <cstring>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

namespace Engine::Core {

// Virtual Memory Implementation
void* VirtualMemory::Reserve(size_t sizeInBytes) {
    if (sizeInBytes == 0) return nullptr;
#if defined(_WIN32)
    void* ptr = VirtualAlloc(nullptr, sizeInBytes, MEM_RESERVE, PAGE_NOACCESS);
#else
    void* ptr = mmap(nullptr, sizeInBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) ptr = nullptr;
#endif
    return ptr;
}

bool VirtualMemory::Commit(void* address, size_t sizeInBytes) {
    if (!address || sizeInBytes == 0) return false;
#if defined(_WIN32)
    void* ptr = VirtualAlloc(address, sizeInBytes, MEM_COMMIT, PAGE_READWRITE);
    return ptr != nullptr;
#else
    return mprotect(address, sizeInBytes, PROT_READ | PROT_WRITE) == 0;
#endif
}

void VirtualMemory::Decommit(void* address, size_t sizeInBytes) {
    if (!address || sizeInBytes == 0) return;
#if defined(_WIN32)
    VirtualFree(address, sizeInBytes, MEM_DECOMMIT);
#else
    mprotect(address, sizeInBytes, PROT_NONE);
#endif
}

void VirtualMemory::Release(void* address, size_t sizeInBytes) {
    if (!address) return;
#if defined(_WIN32)
    (void)sizeInBytes;
    VirtualFree(address, 0, MEM_RELEASE);
#else
    munmap(address, sizeInBytes);
#endif
}

size_t VirtualMemory::GetPageSize() {
#if defined(_WIN32)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return static_cast<size_t>(sysInfo.dwPageSize);
#else
    return static_cast<size_t>(sysconf(_SC_PAGESIZE));
#endif
}

// Linear Allocator Implementation
LinearAllocator::LinearAllocator(size_t capacityInBytes) {
    Initialize(capacityInBytes);
}

LinearAllocator::~LinearAllocator() {
    Shutdown();
}

LinearAllocator::LinearAllocator(LinearAllocator&& other) noexcept
    : m_memory(other.m_memory)
    , m_capacity(other.m_capacity)
    , m_offset(other.m_offset)
    , m_ownsMemory(other.m_ownsMemory)
{
    other.m_memory = nullptr;
    other.m_capacity = 0;
    other.m_offset = 0;
    other.m_ownsMemory = false;
}

LinearAllocator& LinearAllocator::operator=(LinearAllocator&& other) noexcept {
    if (this != &other) {
        Shutdown();
        m_memory = other.m_memory;
        m_capacity = other.m_capacity;
        m_offset = other.m_offset;
        m_ownsMemory = other.m_ownsMemory;

        other.m_memory = nullptr;
        other.m_capacity = 0;
        other.m_offset = 0;
        other.m_ownsMemory = false;
    }
    return *this;
}

void LinearAllocator::Initialize(size_t capacityInBytes) {
    Shutdown();
    m_capacity = capacityInBytes;
    m_offset = 0;
    if (m_capacity > 0) {
        m_memory = static_cast<u8*>(VirtualMemory::Reserve(m_capacity));
        ENGINE_VERIFY(m_memory != nullptr, "Failed to reserve virtual memory for LinearAllocator");
        ENGINE_VERIFY(VirtualMemory::Commit(m_memory, m_capacity), "Failed to commit memory for LinearAllocator");
        m_ownsMemory = true;
        ENGINE_PROFILE_ALLOC(m_memory, m_capacity);
    }
}

void LinearAllocator::Shutdown() {
    if (m_memory && m_ownsMemory) {
        ENGINE_PROFILE_FREE(m_memory);
        VirtualMemory::Release(m_memory, m_capacity);
        m_memory = nullptr;
    }
    m_capacity = 0;
    m_offset = 0;
    m_ownsMemory = false;
}

void* LinearAllocator::Allocate(size_t sizeInBytes, size_t alignment) {
    ENGINE_ASSERT(IsPowerOfTwo(alignment), "Alignment must be a power of two");
    
    const uintptr_t currentAddress = reinterpret_cast<uintptr_t>(m_memory + m_offset);
    const uintptr_t alignedAddress = AlignUp(currentAddress, alignment);
    const size_t padding = alignedAddress - currentAddress;

    if (m_offset + padding + sizeInBytes > m_capacity) {
        ENGINE_ASSERT(false, "LinearAllocator overflow: Out of memory");
        return nullptr;
    }

    m_offset += (padding + sizeInBytes);
    return reinterpret_cast<void*>(alignedAddress);
}

void LinearAllocator::Reset() {
    m_offset = 0;
}

// Frame Allocator Implementation
FrameAllocator::FrameAllocator(size_t capacityPerFrameBytes) {
    m_arenas[0].Initialize(capacityPerFrameBytes);
    m_arenas[1].Initialize(capacityPerFrameBytes);
}

void FrameAllocator::BeginFrame() {
    m_currentFrameIndex = 1 - m_currentFrameIndex;
    m_arenas[m_currentFrameIndex].Reset();
}

void* FrameAllocator::Allocate(size_t sizeInBytes, size_t alignment) {
    return m_arenas[m_currentFrameIndex].Allocate(sizeInBytes, alignment);
}

// Pool Allocator Implementation
PoolAllocator::PoolAllocator(size_t elementSize, size_t elementAlignment, size_t capacity) {
    Initialize(elementSize, elementAlignment, capacity);
}

PoolAllocator::~PoolAllocator() {
    Shutdown();
}

void PoolAllocator::Initialize(size_t elementSize, size_t elementAlignment, size_t capacity) {
    Shutdown();

    ENGINE_ASSERT(capacity > 0, "Pool capacity must be greater than zero");
    ENGINE_ASSERT(IsPowerOfTwo(elementAlignment), "Alignment must be power of two");

    // Element size must be at least the size of a FreeNode pointer
    m_elementSize = std::max(elementSize, sizeof(FreeNode));
    m_elementSize = AlignUp(m_elementSize, elementAlignment);
    m_elementAlignment = elementAlignment;
    m_capacity = capacity;
    m_allocatedCount = 0;

    const size_t totalSize = m_elementSize * m_capacity;
    m_memory = static_cast<u8*>(VirtualMemory::Reserve(totalSize));
    ENGINE_VERIFY(m_memory != nullptr, "Failed to reserve memory for PoolAllocator");
    ENGINE_VERIFY(VirtualMemory::Commit(m_memory, totalSize), "Failed to commit memory for PoolAllocator");

    // Build the free list links
    m_freeList = reinterpret_cast<FreeNode*>(m_memory);
    FreeNode* current = m_freeList;
    for (size_t i = 0; i < m_capacity - 1; ++i) {
        FreeNode* next = reinterpret_cast<FreeNode*>(m_memory + (i + 1) * m_elementSize);
        current->next = next;
        current = next;
    }
    current->next = nullptr;
}

void PoolAllocator::Shutdown() {
    if (m_memory) {
        VirtualMemory::Release(m_memory, m_elementSize * m_capacity);
        m_memory = nullptr;
    }
    m_freeList = nullptr;
    m_elementSize = 0;
    m_capacity = 0;
    m_allocatedCount = 0;
}

void* PoolAllocator::Allocate() {
    if (!m_freeList) {
        ENGINE_ASSERT(false, "PoolAllocator out of elements");
        return nullptr;
    }

    FreeNode* node = m_freeList;
    m_freeList = m_freeList->next;
    m_allocatedCount++;

    return reinterpret_cast<void*>(node);
}

void PoolAllocator::Deallocate(void* ptr) {
    if (!ptr) return;

    ENGINE_ASSERT(ptr >= m_memory && ptr < (m_memory + m_elementSize * m_capacity), "Pointer out of bounds for PoolAllocator");

    FreeNode* node = reinterpret_cast<FreeNode*>(ptr);
    node->next = m_freeList;
    m_freeList = node;
    m_allocatedCount--;
}

} // namespace Engine::Core
