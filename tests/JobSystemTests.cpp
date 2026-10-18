#include "engine/core/JobSystem.h"
#include "engine/core/Assert.h"
#include "engine/core/Log.h"
#include <atomic>
#include <thread>
#include <vector>

void TestLinearAllocator();
void TestFrameAllocator();
void TestPoolAllocator();

void TestJobSystem() {
    using namespace Engine::Core;
    ENGINE_LOG_INFO("--- Testing JobSystem ---");

    auto& jobSystem = JobSystem::Get();
    jobSystem.Initialize(4);

    std::atomic<u32> counter{0};
    constexpr u32 TOTAL_JOBS = 10000;

    JobCounter jobCounter;

    struct JobData {
        std::atomic<u32>* targetCounter;
    };

    auto jobFunc = [](void* raw) {
        auto* data = static_cast<JobData*>(raw);
        data->targetCounter->fetch_add(1, std::memory_order_relaxed);
    };

    std::vector<JobData> jobsData(TOTAL_JOBS, JobData{&counter});

    for (u32 i = 0; i < TOTAL_JOBS; ++i) {
        ENGINE_VERIFY(jobSystem.SubmitJob(jobFunc, &jobsData[i], &jobCounter), "JobSystem rejected a job while running");
    }

    jobSystem.WaitForCounter(&jobCounter);
    ENGINE_ASSERT(counter.load() == TOTAL_JOBS, "Job count does not match total submitted");
    ENGINE_LOG_INFO("Single jobs test passed: {}/{} processed.", counter.load(), TOTAL_JOBS);

    // ParallelFor test
    constexpr u32 ARRAY_SIZE = 50000;
    std::vector<u32> testArray(ARRAY_SIZE, 0);

    JobCounter pforCounter;
    jobSystem.ParallelFor(
        ARRAY_SIZE,
        1000,
        [](u32 idx, void* data) {
            auto* arr = static_cast<std::vector<u32>*>(data);
            (*arr)[idx] = idx * 2;
        },
        &testArray,
        &pforCounter
    );

    jobSystem.WaitForCounter(&pforCounter);

    for (u32 i = 0; i < ARRAY_SIZE; ++i) {
        ENGINE_ASSERT(testArray[i] == i * 2, "ParallelFor value error");
    }

    ENGINE_LOG_INFO("ParallelFor test passed on {} elements.", ARRAY_SIZE);

    jobSystem.Shutdown();

    std::atomic<u32> drainedJobs{0};
    jobSystem.Initialize(2);
    for (u32 i = 0; i < 10000; ++i) {
        ENGINE_VERIFY(jobSystem.SubmitJob([](void* raw) {
            static_cast<std::atomic<u32>*>(raw)->fetch_add(1, std::memory_order_relaxed);
        }, &drainedJobs), "JobSystem rejected a drain-test job while running");
    }
    jobSystem.Shutdown();
    ENGINE_ASSERT(drainedJobs.load(std::memory_order_relaxed) == 10000, "JobSystem shutdown dropped queued jobs");
    ENGINE_ASSERT(!jobSystem.SubmitJob(jobFunc, nullptr), "Stopped JobSystem accepted a job");

    // Shutdown must keep queues alive while an external waiter is still draining them.
    jobSystem.Initialize(2);
    struct ShutdownGate {
        std::atomic<bool> entered{false};
        std::atomic<bool> release{false};
    } gate;
    JobCounter shutdownCounter;
    ENGINE_VERIFY(jobSystem.SubmitJob([](void* raw) {
        auto* gate = static_cast<ShutdownGate*>(raw);
        gate->entered.store(true, std::memory_order_release);
        while (!gate->release.load(std::memory_order_acquire)) std::this_thread::yield();
    }, &gate, &shutdownCounter), "JobSystem rejected shutdown-gate job");
    while (!gate.entered.load(std::memory_order_acquire)) std::this_thread::yield();

    std::thread waiter([&] { jobSystem.WaitForCounter(&shutdownCounter); });
    std::thread shutdown([&] { jobSystem.Shutdown(); });
    while (jobSystem.IsRunning()) std::this_thread::yield();
    gate.release.store(true, std::memory_order_release);
    waiter.join();
    shutdown.join();
    ENGINE_ASSERT(shutdownCounter.IsComplete(), "JobSystem shutdown waiter did not observe completion");

    std::vector<u32> synchronousFallback(16, 0);
    jobSystem.ParallelFor(static_cast<u32>(synchronousFallback.size()), 3,
        [](u32 index, void* raw) {
            (*static_cast<std::vector<u32>*>(raw))[index] = index + 1;
        }, &synchronousFallback);
    for (u32 i = 0; i < synchronousFallback.size(); ++i) {
        ENGINE_ASSERT(synchronousFallback[i] == i + 1, "Stopped JobSystem ParallelFor fallback missed work");
    }
    ENGINE_LOG_INFO("JobSystem passed all checks.");
}

void TestLinearAllocator();
void TestFrameAllocator();
void TestPoolAllocator();
void RunRenderGraphUnitTests();
void RunWorldUnitTests();
void RunAssetUnitTests();
void RunPhysicsUnitTests();
void RunEditorUnitTests();
void RunGPUDrivenUnitTests();
void RunAnimationAudioUnitTests();
void RunProfilerUnitTests();
void RunDistributionUnitTests();
void RunNextGenRendererUnitTests();
void RunRayTracingUnitTests();
void RunMeshletGIUnitTests();
void RunNetworkRollbackTests();
void RunNeuralRadianceCacheUnitTests();
void RunWorldStreamingUnitTests();
void RunMeshShaderCullingUnitTests();
void RunAtmosphericSkyUnitTests();
void RunDirectStorageUnitTests();

int main() {
    using namespace Engine::Core;
    ENGINE_LOG_INFO("Starting Engine Core Unit Tests...\n");

    TestLinearAllocator();
    TestFrameAllocator();
    TestPoolAllocator();
    TestJobSystem();
    RunAssetUnitTests();
    RunPhysicsUnitTests();
    RunEditorUnitTests();
    RunRenderGraphUnitTests();
    RunGPUDrivenUnitTests();
    RunAnimationAudioUnitTests();
    RunProfilerUnitTests();
    RunDistributionUnitTests();
    RunNextGenRendererUnitTests();
    RunRayTracingUnitTests();
    RunMeshletGIUnitTests();
    RunNetworkRollbackTests();
    RunNeuralRadianceCacheUnitTests();
    RunWorldStreamingUnitTests();
    RunMeshShaderCullingUnitTests();
    RunAtmosphericSkyUnitTests();
    RunDirectStorageUnitTests();
    RunWorldUnitTests();
    ENGINE_LOG_INFO("\nALL CPU ENGINE, ASSET, PHYSICS, EDITOR, RENDERER-METADATA & WORLD CHECKS PASSED SUCCESSFULLY!");
    return 0;
}
