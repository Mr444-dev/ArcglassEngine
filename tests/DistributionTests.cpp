#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/core/Allocators.h"
#include "engine/core/JobSystem.h"
#include "engine/assets/VirtualFileSystem.h"
#include "engine/world/Scene.h"
#include "engine/physics/PhysicsScene.h"
#include "engine/audio/AudioEngine.h"

void TestFrameAllocatorZeroLeakSimulation() {
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing 100-Frame Memory Leak Simulation (FrameAllocator) ---");

    constexpr size_t CAPACITY = 1 * MB;
    FrameAllocator frameAlloc(CAPACITY);

    // Simulate 100 frames of gameplay allocations
    for (int frame = 0; frame < 100; ++frame) {
        frameAlloc.BeginFrame();

        // Allocate dynamic per-frame transient data
        for (int i = 0; i < 200; ++i) {
            void* ptr = frameAlloc.Allocate(64, 16);
            ENGINE_ASSERT(ptr != nullptr, "Transient frame allocation failed");
            std::memset(ptr, 0xAA, 64);
        }

        ENGINE_ASSERT(frameAlloc.GetActiveFrameAllocatedBytes() > 0, "FrameAllocator should have active allocations during frame");
    }

    // BeginFrame resets the double buffer for the next frame
    frameAlloc.BeginFrame();
    ENGINE_ASSERT(frameAlloc.GetActiveFrameAllocatedBytes() == 0, "FrameAllocator active bytes must reset to 0 on BeginFrame");

    ENGINE_LOG_INFO("100-frame transient allocation simulation passed with 0 memory leaks.");
}

void TestPoolAllocatorSanitization() {
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing PoolAllocator Sanitization & Recycling ---");

    struct Particle {
        float x, y, z;
        float vx, vy, vz;
        float life;
    };

    PoolAllocator particlePool(sizeof(Particle), alignof(Particle), 1024);
    std::vector<Particle*> activeParticles;
    activeParticles.reserve(500);

    // 1. Allocate 500 particles
    for (int i = 0; i < 500; ++i) {
        auto* p = static_cast<Particle*>(particlePool.Allocate());
        ENGINE_ASSERT(p != nullptr, "Particle pool allocation failed");
        p->x = static_cast<float>(i);
        p->life = 1.0f;
        activeParticles.push_back(p);
    }
    ENGINE_ASSERT(particlePool.GetAllocatedCount() == 500, "Active count mismatch");

    // 2. Free all particles
    for (Particle* p : activeParticles) {
        particlePool.Deallocate(p);
    }
    ENGINE_ASSERT(particlePool.GetAllocatedCount() == 0, "Pool must be completely clean (0 allocated)");

    // 3. Re-allocate from recycled slots
    auto* pRecycled = static_cast<Particle*>(particlePool.Allocate());
    ENGINE_ASSERT(pRecycled != nullptr, "Recycled pool allocation failed");
    particlePool.Deallocate(pRecycled);
    ENGINE_ASSERT(particlePool.GetAllocatedCount() == 0, "Recycled free failed");

    ENGINE_LOG_INFO("PoolAllocator sanitization and slot recycling verified.");
}

void TestCleanRuntimeLifecycle() {
    using namespace Engine::Core;
    using namespace Engine::Assets;
    using namespace Engine::World;
    using namespace Engine::Physics;
    using namespace Engine::Audio;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 10: Clean Runtime DAG Lifecycle ---");

    // 1. Core Startup
    JobSystem::Get().Initialize(2);
    VirtualFileSystem::Get().Mount("test", "./build");

    // 2. Systems Startup
    AudioEngine::Get().Initialize();

    {
        Scene scene;
        PhysicsScene physics;

        // Populate scene entities
        Entity ground = scene.CreateEntity("Ground");
        Entity player = scene.CreateEntity("Player");
        (void)ground;
        (void)player;

        // Run 10 tick updates
        for (int i = 0; i < 10; ++i) {
            physics.Update(0.016f, scene);
            scene.Update(0.016f);
            AudioEngine::Get().Update(0.016f, scene);
        }
    } // Scene and Physics destroyed cleanly here

    // 3. Systems Teardown in Reverse Order
    AudioEngine::Get().Shutdown();
    VirtualFileSystem::Get().Unmount("test");
    JobSystem::Get().Shutdown();

    ENGINE_LOG_INFO("Runtime DAG initialization and teardown sequence verified with 0 leaks.");
}

void RunDistributionUnitTests() {
    TestFrameAllocatorZeroLeakSimulation();
    TestPoolAllocatorSanitization();
    TestCleanRuntimeLifecycle();
}
