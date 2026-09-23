#include "engine/core/Allocators.h"
#include "engine/core/Assert.h"
#include "engine/core/Log.h"
#include <iostream>
#include <vector>

void TestLinearAllocator() {
    using namespace Engine::Core;
    ENGINE_LOG_INFO("--- Testing LinearAllocator ---");

    LinearAllocator allocator(1 * MB);

    void* p1 = allocator.Allocate(128, 16);
    ENGINE_ASSERT(p1 != nullptr, "Allocation failed");
    ENGINE_ASSERT(reinterpret_cast<uintptr_t>(p1) % 16 == 0, "Alignment check failed");

    void* p2 = allocator.Allocate(256, 64);
    ENGINE_ASSERT(p2 != nullptr, "Allocation failed");
    ENGINE_ASSERT(reinterpret_cast<uintptr_t>(p2) % 64 == 0, "Alignment check failed");

    struct Particle {
        float x, y, z;
        int id;
    };
    Particle* particle = allocator.Create<Particle>(1.0f, 2.0f, 3.0f, 42);
    ENGINE_ASSERT(particle->id == 42, "Placement new failed");

    ENGINE_LOG_INFO("LinearAllocator allocated {} bytes. Resetting...", allocator.GetAllocatedBytes());
    allocator.Reset();
    ENGINE_ASSERT(allocator.GetAllocatedBytes() == 0, "Reset failed");

    ENGINE_LOG_INFO("LinearAllocator passed all checks.");
}

void TestFrameAllocator() {
    using namespace Engine::Core;
    ENGINE_LOG_INFO("--- Testing FrameAllocator ---");

    FrameAllocator frameAlloc(512 * KB);

    // Frame 0
    frameAlloc.BeginFrame();
    int* val1 = frameAlloc.Create<int>(100);
    ENGINE_ASSERT(*val1 == 100, "FrameAlloc value mismatch");

    // Frame 1
    frameAlloc.BeginFrame();
    int* val2 = frameAlloc.Create<int>(200);
    ENGINE_ASSERT(*val2 == 200, "FrameAlloc value mismatch");

    ENGINE_LOG_INFO("FrameAllocator passed all checks.");
}

void TestPoolAllocator() {
    using namespace Engine::Core;
    ENGINE_LOG_INFO("--- Testing PoolAllocator ---");

    struct Node {
        u64 a, b, c;
    };

    constexpr size_t POOL_CAPACITY = 100;
    PoolAllocator pool(sizeof(Node), alignof(Node), POOL_CAPACITY);

    std::vector<void*> allocatedPointers;
    allocatedPointers.reserve(POOL_CAPACITY);

    for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        void* ptr = pool.Allocate();
        ENGINE_ASSERT(ptr != nullptr, "Pool allocation returned null");
        allocatedPointers.push_back(ptr);
    }
    ENGINE_ASSERT(pool.GetAllocatedCount() == POOL_CAPACITY, "Capacity count mismatch");

    // Deallocate half
    for (size_t i = 0; i < 50; ++i) {
        pool.Deallocate(allocatedPointers[i]);
    }
    ENGINE_ASSERT(pool.GetAllocatedCount() == 50, "Deallocate count mismatch");

    // Reallocate
    for (size_t i = 0; i < 50; ++i) {
        void* ptr = pool.Allocate();
        ENGINE_ASSERT(ptr != nullptr, "Pool reallocation returned null");
    }
    ENGINE_ASSERT(pool.GetAllocatedCount() == POOL_CAPACITY, "Reallocate count mismatch");

    ENGINE_LOG_INFO("PoolAllocator passed all checks.");
}
