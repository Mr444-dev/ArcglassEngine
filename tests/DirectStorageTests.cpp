#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/assets/DirectStorageManager.h"
#include <vector>

void TestDirectStorageInitializationAndCaps() {
    using namespace Engine::Assets;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing DirectStorage 1.2 Initialization & Capabilities ---");

    DirectStorageManager dsManager;
    ENGINE_ASSERT(!dsManager.IsInitialized(), "Must not be initialized prior to Initialize()");

    const bool initOk = dsManager.Initialize(64);
    (void)initOk;
    ENGINE_ASSERT(initOk, "DirectStorageManager initialization failed");
    ENGINE_ASSERT(dsManager.IsInitialized(), "DirectStorageManager must report initialized");
    ENGINE_ASSERT(dsManager.GetQueueCapacity() == 64, "Queue capacity must match configured value");
    ENGINE_ASSERT(dsManager.IsGDeflateSupported(), "GDeflate hardware GPU decompression must be supported");
    ENGINE_ASSERT(dsManager.GetCompletedFenceValue() == 0, "Initial completion fence must be 0");

    dsManager.Shutdown();
    ENGINE_ASSERT(!dsManager.IsInitialized(), "Must be uninitialized after Shutdown()");

    ENGINE_LOG_INFO("DirectStorage 1.2 capabilities & initialization verified.");
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

    // 2. GDeflate-compressed GPU streaming request (1024 bytes -> 4096 bytes)
    DSRequest req2{};
    req2.sourcePath = "assets://textures/hero_albedo.ctex";
    req2.sourceSize = 1024;
    req2.uncompressedSize = 4096;
    req2.compressionFormat = DSCompressionFormat::GDeflate;
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

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 21: DirectStorage 1.2 GPU Decompression & Streaming ---");

    DirectStorageManager dsManager;
    (void)dsManager.Initialize(64);

    std::vector<u8> destBuffer(8192, 0);

    DSRequest req{};
    req.sourcePath = "assets://scenes/world_chunk_01.cmesh";
    req.sourceSize = 2048;
    req.uncompressedSize = 8192;
    req.compressionFormat = DSCompressionFormat::GDeflate;
    req.destinationCpuBuffer = destBuffer.data();

    (void)dsManager.EnqueueRequest(req);
    u64 fence = dsManager.SubmitQueue();

    // Synchronize completion
    dsManager.WaitForFence(fence);

    ENGINE_ASSERT(dsManager.GetCompletedFenceValue() == fence, "Completed fence must match submitted fence");
    ENGINE_ASSERT(dsManager.GetInFlightCount() == 0, "In-flight count must be 0 after fence completion");

    const auto& stats = dsManager.GetStats();
    (void)stats;
    ENGINE_ASSERT(stats.totalRequestsSubmitted == 1, "Expected 1 submitted request");
    ENGINE_ASSERT(stats.totalBytesRead == 2048, "Expected 2048 bytes read");
    ENGINE_ASSERT(stats.totalBytesDecompressed == 8192, "Expected 8192 bytes decompressed (4x GDeflate ratio)");

    dsManager.Shutdown();
    ENGINE_LOG_INFO("DirectStorage GDeflate completion and 4x decompression bandwidth statistics verified.");
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

    dsManager.Shutdown();
    ENGINE_LOG_INFO("DirectStorage guardrails correctly intercepted corrupt & invalid streaming requests.");
}

void RunDirectStorageUnitTests() {
    TestDirectStorageInitializationAndCaps();
    TestDirectStorageRequestEnqueueAndBatching();
    TestDirectStorageFenceCompletionAndStats();
    TestDirectStorageCorruptRequestGuardrails();
}
