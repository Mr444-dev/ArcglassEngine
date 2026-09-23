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
    Core::u64 totalBytesRead{0};
    Core::u64 totalBytesDecompressed{0};
    Core::u32 inFlightBatches{0};
    float lastBatchBandwidthGBs{0.0f};
};

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

    // Enqueues an uncompressed or GDeflate-compressed streaming request directly into memory/VRAM
    [[nodiscard]] Core::u64 EnqueueRequest(const DSRequest& request);

    // Submits all currently enqueued requests to the hardware NVMe queue and advances the completion fence
    [[nodiscard]] Core::u64 SubmitQueue();

    // Synchronizes until the specified fence value has completed execution
    void WaitForFence(Core::u64 fenceValue);

    // Simulates or handles asynchronous completion of in-flight batch
    void PollCompletions();

private:
    bool m_initialized{false};
    bool m_supportsGDeflate{true};
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
