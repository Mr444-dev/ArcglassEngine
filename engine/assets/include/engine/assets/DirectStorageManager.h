#pragma once

#include "engine/core/BaseTypes.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Engine::Assets {

enum class DSCompressionFormat : Core::u8 {
    None = 0,
    GDeflate = 1
};

enum class DSRequestStatus : Core::u8 {
    Pending = 0,
    Enqueued = 1,
    InFlight = 2,
    Completed = 3,
    Failed = 4
};

struct DSRequest {
    Core::u64 requestId{0};
    std::string sourcePath;
    Core::u64 sourceOffset{0};
    Core::u64 sourceSize{0};
    Core::u64 uncompressedSize{0};
    DSCompressionFormat compressionFormat{DSCompressionFormat::None};

    void* destinationCpuBuffer{nullptr};
    Core::u64 destinationGpuVirtualAddress{0};
    DSRequestStatus status{DSRequestStatus::Pending};
};

struct DirectStorageStats {
    Core::u64 totalRequestsSubmitted{0};
    Core::u32 totalBatchesSubmitted{0};
    Core::u32 inFlightRequests{0};
};

// CPU-side queue-lifecycle prototype only. This class does not call the
// DirectStorage API, read source files, decompress GDeflate, or write to VRAM.
class DirectStorageManager {
public:
    DirectStorageManager() = default;
    ~DirectStorageManager();

    [[nodiscard]] bool Initialize(Core::u32 queueCapacity = 128) noexcept;
    void Shutdown() noexcept;

    [[nodiscard]] bool IsInitialized() const noexcept { return m_initialized; }
    [[nodiscard]] bool IsGDeflateSupported() const noexcept { return m_supportsGDeflate; }
    [[nodiscard]] Core::u32 GetQueueCapacity() const noexcept { return m_queueCapacity; }
    [[nodiscard]] Core::u32 GetInFlightCount() const noexcept { return m_inFlightCount; }
    [[nodiscard]] Core::u64 GetCompletedFenceValue() const noexcept { return m_completedFenceValue; }
    [[nodiscard]] const DirectStorageStats& GetStats() const noexcept { return m_stats; }

    // Enqueues an uncompressed request record for queue-lifecycle testing.
    // GDeflate requests are rejected until a native DirectStorage backend exists.
    [[nodiscard]] Core::u64 EnqueueRequest(const DSRequest& request);

    // Moves queued request records into the simulated in-flight state.
    [[nodiscard]] Core::u64 SubmitQueue();

    // Immediately completes simulated work through the requested fence value.
    void WaitForFence(Core::u64 fenceValue);

    // Completes the current simulated in-flight batch.
    void PollCompletions();

private:
    bool m_initialized{false};
    bool m_supportsGDeflate{false};
    Core::u32 m_queueCapacity{128};
    Core::u32 m_inFlightCount{0};
    Core::u64 m_nextRequestId{1};
    Core::u64 m_currentFenceValue{0};
    Core::u64 m_completedFenceValue{0};

    std::vector<DSRequest> m_enqueuedRequests;
    std::vector<DSRequest> m_inFlightRequests;
    DirectStorageStats m_stats;
};

} // namespace Engine::Assets
