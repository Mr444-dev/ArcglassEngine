#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/assets/DirectStorageManager.h"
#include <vector>

void TestDirectStorageInitializationAndCaps() {
    using namespace Engine::Assets;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing DirectStorage Queue Simulation Initialization ---");

    DirectStorageManager dsManager;
    ENGINE_ASSERT(!dsManager.IsInitialized(), "Must not be initialized prior to Initialize()");

    const bool initOk = dsManager.Initialize(64);
    (void)initOk;
    ENGINE_ASSERT(initOk, "DirectStorageManager initialization failed");
    ENGINE_ASSERT(dsManager.IsInitialized(), "DirectStorageManager must report initialized");
    ENGINE_ASSERT(dsManager.GetQueueCapacity() == 64, "Queue capacity must match configured value");
    ENGINE_ASSERT(!dsManager.IsGDeflateSupported(), "A queue simulator must not claim native GDeflate support");
    ENGINE_ASSERT(dsManager.GetCompletedFenceValue() == 0, "Initial completion fence must be 0");

    dsManager.Shutdown();
    ENGINE_ASSERT(!dsManager.IsInitialized(), "Must be uninitialized after Shutdown()");

    ENGINE_LOG_INFO("Queue simulation initialized and correctly reports no native DirectStorage backend.");
}

void TestDirectStorageRequestEnqueueAndBatching() {
    using namespace Engine::Assets;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing DirectStorage Request Enqueue & Batching ---");

    DirectStorageManager dsManager;
    (void)dsManager.Initialize(32);

    std::vector<u8> dummyDestBuffer1(1024, 0);
    std::vector<u8> dummyDestBuffer2(4096, 0);

    // 1. Uncompressed streaming request
    DSRequest req1{};
    req1.sourcePath = "assets://models/hero.cmesh";
    req1.sourceSize = 1024;
    req1.uncompressedSize = 1024;
    req1.compressionFormat = DSCompressionFormat::None;
    req1.destinationCpuBuffer = dummyDestBuffer1.data();

    u64 id1 = dsManager.EnqueueRequest(req1);
    (void)id1;
    ENGINE_ASSERT(id1 > 0, "Request 1 ID must be valid");

    // 2. A second uncompressed request exercises batching without claiming GPU I/O.
    DSRequest req2{};
    req2.sourcePath = "assets://textures/hero_albedo.ctex";
    req2.sourceSize = 1024;
    req2.uncompressedSize = 1024;
    req2.compressionFormat = DSCompressionFormat::None;
    req2.destinationGpuVirtualAddress = 0x20000000;

    u64 id2 = dsManager.EnqueueRequest(req2);
    (void)id2;
    ENGINE_ASSERT(id2 > id1, "Request 2 ID must be strictly monotonic");

    ENGINE_ASSERT(dsManager.GetInFlightCount() == 0, "Requests must remain in enqueued state prior to submit");

    // Submit batch
    u64 fenceValue = dsManager.SubmitQueue();
    (void)fenceValue;
    ENGINE_ASSERT(fenceValue == 1, "First submitted batch must return fence value 1");
    ENGINE_ASSERT(dsManager.GetInFlightCount() == 2, "In-flight count must equal batch size (2)");

    dsManager.Shutdown();
    ENGINE_LOG_INFO("DirectStorage batch queue enqueue & submit verified.");
}

void TestDirectStorageFenceCompletionAndStats() {
    using namespace Engine::Assets;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Simulated Queue Completion & Counters ---");

    DirectStorageManager dsManager;
    (void)dsManager.Initialize(64);

    std::vector<u8> destBuffer(8192, 0);

    DSRequest req{};
    req.sourcePath = "assets://scenes/world_chunk_01.cmesh";
    req.sourceSize = 2048;
    req.uncompressedSize = 2048;
    req.compressionFormat = DSCompressionFormat::None;
    req.destinationCpuBuffer = destBuffer.data();

    (void)dsManager.EnqueueRequest(req);
    u64 fence = dsManager.SubmitQueue();

    // Synchronize completion
    dsManager.WaitForFence(fence);

    ENGINE_ASSERT(dsManager.GetCompletedFenceValue() == fence, "Completed fence must match submitted fence");
    ENGINE_ASSERT(dsManager.GetInFlightCount() == 0, "In-flight count must be 0 after fence completion");

    const auto& stats = dsManager.GetStats();
    (void)stats;
    ENGINE_ASSERT(stats.totalRequestsSubmitted == 1, "Expected one request to enter the simulated batch");
    ENGINE_ASSERT(stats.totalBatchesSubmitted == 1, "Expected one simulated batch");
    ENGINE_ASSERT(stats.inFlightRequests == 0, "Completed requests must not remain in flight");

    dsManager.Shutdown();
    ENGINE_LOG_INFO("Simulated queue completion and request counters verified; no I/O or decompression is claimed.");
}

void TestDirectStorageCorruptRequestGuardrails() {
    using namespace Engine::Assets;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing DirectStorage Defensive Guardrails ---");

    DirectStorageManager dsManager;
    (void)dsManager.Initialize(32);

    u8 dummy = 0;

    // 1. Zero size
    DSRequest zeroSizeReq{};
    zeroSizeReq.sourcePath = "invalid.dat";
    zeroSizeReq.sourceSize = 0;
    zeroSizeReq.destinationCpuBuffer = &dummy;
    u64 id1 = dsManager.EnqueueRequest(zeroSizeReq);
    (void)id1;
    ENGINE_ASSERT(id1 == 0, "Zero source size request must be rejected");

    // 2. Null destination
    DSRequest nullDestReq{};
    nullDestReq.sourcePath = "valid.dat";
    nullDestReq.sourceSize = 100;
    nullDestReq.destinationCpuBuffer = nullptr;
    nullDestReq.destinationGpuVirtualAddress = 0;
    u64 id2 = dsManager.EnqueueRequest(nullDestReq);
    (void)id2;
    ENGINE_ASSERT(id2 == 0, "Null destination request must be rejected");

    // 3. Invalid GDeflate uncompressed size < source size
    DSRequest invalidGDeflateReq{};
    invalidGDeflateReq.sourcePath = "corrupt.gdeflate";
    invalidGDeflateReq.sourceSize = 500;
    invalidGDeflateReq.uncompressedSize = 200; // Impossible
    invalidGDeflateReq.compressionFormat = DSCompressionFormat::GDeflate;
    invalidGDeflateReq.destinationCpuBuffer = &dummy;
    u64 id3 = dsManager.EnqueueRequest(invalidGDeflateReq);
    (void)id3;
    ENGINE_ASSERT(id3 == 0, "Corrupt GDeflate ratio request must be rejected");

    // A valid GDeflate request is also rejected because this prototype has no native backend.
    DSRequest unsupportedGDeflateReq{};
    unsupportedGDeflateReq.sourcePath = "unsupported.gdeflate";
    unsupportedGDeflateReq.sourceSize = 200;
    unsupportedGDeflateReq.uncompressedSize = 500;
    unsupportedGDeflateReq.compressionFormat = DSCompressionFormat::GDeflate;
    unsupportedGDeflateReq.destinationCpuBuffer = &dummy;
    const u64 unsupportedId = dsManager.EnqueueRequest(unsupportedGDeflateReq);
    ENGINE_ASSERT(unsupportedId == 0, "GDeflate must be rejected until a native backend is implemented");

    dsManager.Shutdown();
    ENGINE_LOG_INFO("Queue simulator rejected invalid requests and unsupported GDeflate work.");
}

void RunDirectStorageUnitTests() {
    TestDirectStorageInitializationAndCaps();
    TestDirectStorageRequestEnqueueAndBatching();
    TestDirectStorageFenceCompletionAndStats();
    TestDirectStorageCorruptRequestGuardrails();
}
