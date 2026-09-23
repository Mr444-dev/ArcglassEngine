#include "engine/core/JobSystem.h"
#include "engine/core/Assert.h"
#include "engine/core/Log.h"
#include <atomic>
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
        jobSystem.SubmitJob(jobFunc, &jobsData[i], &jobCounter);
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
    ENGINE_LOG_INFO("JobSystem passed all checks.");
}

void TestLinearAllocator();
void TestFrameAllocator();
void TestPoolAllocator();
void RunRHIGraphicsTest();
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
    RunRHIGraphicsTest();

    ENGINE_LOG_INFO("\nALL ENGINE CORE, ASSETS, PHYSICS, EDITOR, GPU-DRIVEN, ANIMATION, AUDIO, PROFILER, DISTRIBUTION, NEXT-GEN RENDERER, RAY TRACING, MESHLET GI, DIRECTSTORAGE, RHI, RENDERER & WORLD TESTS PASSED SUCCESSFULLY!");
    return 0;
}
