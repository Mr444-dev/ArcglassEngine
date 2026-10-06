#include "engine/assets/DirectStorageManager.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include <algorithm>

namespace Engine::Assets {

DirectStorageManager::~DirectStorageManager() {
    Shutdown();
}

bool DirectStorageManager::Initialize(Core::u32 queueCapacity) noexcept {
    if (m_initialized) {
        ENGINE_LOG_WARN("DirectStorageManager is already initialized.");
        return true;
    }

    m_queueCapacity = std::max(16u, queueCapacity);
    m_supportsGDeflate = false;
    m_enqueuedRequests.reserve(m_queueCapacity);
    m_inFlightRequests.reserve(m_queueCapacity);

    m_currentFenceValue = 0;
    m_completedFenceValue = 0;
    m_inFlightCount = 0;
    m_stats = {};

    m_initialized = true;
    ENGINE_LOG_INFO("DirectStorage queue simulation initialized (capacity: {}; native backend: unavailable).", m_queueCapacity);
    return true;
}

void DirectStorageManager::Shutdown() noexcept {
    if (!m_initialized) return;

    if (!m_inFlightRequests.empty()) {
        WaitForFence(m_currentFenceValue);
    }

    m_enqueuedRequests.clear();
    m_inFlightRequests.clear();
    m_initialized = false;
    ENGINE_LOG_INFO("DirectStorageManager shut down cleanly.");
}

Core::u64 DirectStorageManager::EnqueueRequest(const DSRequest& request) {
    if (!m_initialized) {
        ENGINE_LOG_ERROR("DirectStorageManager::EnqueueRequest: Manager is not initialized!");
        return 0;
    }

    if (request.sourceSize == 0) {
        ENGINE_LOG_ERROR("DirectStorageManager::EnqueueRequest: Invalid source size (0 bytes) for '{}'.", request.sourcePath);
        return 0;
    }

    if (request.destinationCpuBuffer == nullptr && request.destinationGpuVirtualAddress == 0) {
        ENGINE_LOG_ERROR("DirectStorageManager::EnqueueRequest: No valid CPU or GPU destination provided for '{}'.", request.sourcePath);
        return 0;
    }

    if (request.compressionFormat == DSCompressionFormat::GDeflate && request.uncompressedSize < request.sourceSize) {
        ENGINE_LOG_ERROR("DirectStorageManager::EnqueueRequest: GDeflate uncompressed size ({} B) cannot be smaller than compressed source ({} B) for '{}'.",
            request.uncompressedSize, request.sourceSize, request.sourcePath);
        return 0;
    }

    if (request.compressionFormat == DSCompressionFormat::GDeflate && !m_supportsGDeflate) {
        ENGINE_LOG_ERROR("DirectStorageManager::EnqueueRequest: GDeflate requires a native DirectStorage backend; '{}' was rejected.", request.sourcePath);
        return 0;
    }

    if (m_enqueuedRequests.size() >= m_queueCapacity) {
        ENGINE_LOG_WARN("DirectStorageManager::EnqueueRequest: Queue capacity reached ({}/{}). Automatic queue submission triggered.",
            static_cast<Core::u32>(m_enqueuedRequests.size()), m_queueCapacity);
        (void)SubmitQueue();
    }

    DSRequest enqueued = request;
    enqueued.requestId = m_nextRequestId++;
    enqueued.status = DSRequestStatus::Enqueued;
    if (enqueued.uncompressedSize == 0) {
        enqueued.uncompressedSize = enqueued.sourceSize;
    }

    m_enqueuedRequests.push_back(enqueued);
    return enqueued.requestId;
}

Core::u64 DirectStorageManager::SubmitQueue() {
    if (!m_initialized || m_enqueuedRequests.empty()) {
        return m_currentFenceValue;
    }

    ++m_currentFenceValue;
    const Core::u64 batchSize = m_enqueuedRequests.size();

    for (auto& req : m_enqueuedRequests) {
        req.status = DSRequestStatus::InFlight;
        m_inFlightRequests.push_back(req);
    }

    m_stats.totalRequestsSubmitted += batchSize;
    m_stats.totalBatchesSubmitted++;
    m_inFlightCount = static_cast<Core::u32>(m_inFlightRequests.size());
    m_stats.inFlightRequests = m_inFlightCount;

    m_enqueuedRequests.clear();
    return m_currentFenceValue;
}

void DirectStorageManager::WaitForFence(Core::u64 fenceValue) {
    if (!m_initialized || fenceValue <= m_completedFenceValue) {
        return;
    }

    for (auto& req : m_inFlightRequests) {
        req.status = DSRequestStatus::Completed;
    }

    m_completedFenceValue = std::max(m_completedFenceValue, fenceValue);
    m_inFlightRequests.clear();
    m_inFlightCount = 0;
    m_stats.inFlightRequests = 0;
}

void DirectStorageManager::PollCompletions() {
    if (!m_initialized || m_inFlightRequests.empty()) {
        return;
    }

    // This is a queue-lifecycle simulation; no GPU or storage operation is running.
    WaitForFence(m_currentFenceValue);
}

} // namespace Engine::Assets
