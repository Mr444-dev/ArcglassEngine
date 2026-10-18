#include "engine/core/JobSystem.h"
#include "engine/core/Log.h"
#include <algorithm>
#include <chrono>
#include <exception>
#include <memory>

namespace Engine::Core {

namespace {
thread_local u32 t_workerIndex = 0;

struct JobWrapperPayload {
    JobFunction originalFunction{nullptr};
    void* originalData{nullptr};
    JobCounter* counter{nullptr};
};

void JobWrapperCallback(void* rawData) {
    std::unique_ptr<JobWrapperPayload> payload(static_cast<JobWrapperPayload*>(rawData));
    try {
        payload->originalFunction(payload->originalData);
    } catch (const std::exception& error) {
        ENGINE_LOG_ERROR("JobSystem: Job threw an exception: {}", error.what());
    } catch (...) {
        ENGINE_LOG_ERROR("JobSystem: Job threw a non-standard exception.");
    }
    if (payload->counter) payload->counter->Decrement();
}

struct ParallelForPayload {
    void (*function)(u32, void*);
    void* data;
    u32 startIndex;
    u32 endIndex;
    JobCounter* counter;
};

void ParallelForBatchCallback(void* rawData) {
    std::unique_ptr<ParallelForPayload> payload(static_cast<ParallelForPayload*>(rawData));
    try {
        for (u32 index = payload->startIndex; index < payload->endIndex; ++index) {
            payload->function(index, payload->data);
        }
    } catch (const std::exception& error) {
        ENGINE_LOG_ERROR("JobSystem: ParallelFor batch threw an exception: {}", error.what());
    } catch (...) {
        ENGINE_LOG_ERROR("JobSystem: ParallelFor batch threw a non-standard exception.");
    }
    if (payload->counter) payload->counter->Decrement();
}

} // namespace

bool WorkStealingQueue::Push(const Job& job) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_tail - m_head >= CAPACITY) return false;
    m_buffer[m_tail & MASK] = job;
    ++m_tail;
    return true;
}

bool WorkStealingQueue::Pop(Job& outJob) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_tail <= m_head) return false;
    --m_tail;
    outJob = m_buffer[m_tail & MASK];
    return true;
}

bool WorkStealingQueue::Steal(Job& outJob) {
    std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
    if (!lock.owns_lock() || m_tail <= m_head) return false;
    outJob = m_buffer[m_head & MASK];
    ++m_head;
    return true;
}

bool WorkStealingQueue::IsEmpty() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tail <= m_head;
}

JobSystem& JobSystem::Get() {
    static JobSystem instance;
    return instance;
}

JobSystem::~JobSystem() {
    Shutdown();
}

void JobSystem::Initialize(u32 threadCount) {
    std::unique_lock<std::mutex> lock(m_lifecycleMutex);
    m_lifecycleChanged.wait(lock, [this] { return !m_shuttingDown; });
    if (m_running.load(std::memory_order_acquire)) return;

    if (threadCount == 0) {
        const u32 hardwareThreads = std::thread::hardware_concurrency();
        threadCount = (hardwareThreads > 1) ? (hardwareThreads - 1) : 1;
    }

    m_queues = std::vector<WorkStealingQueue>(static_cast<size_t>(threadCount) + 1);
    m_running.store(true, std::memory_order_release);
    try {
        m_workers.reserve(threadCount);
        for (u32 workerIndex = 0; workerIndex < threadCount; ++workerIndex) {
            m_workers.emplace_back(&JobSystem::WorkerLoop, this, workerIndex + 1);
        }
    } catch (const std::exception& error) {
        m_running.store(false, std::memory_order_release);
        for (auto& worker : m_workers) {
            if (worker.joinable()) worker.join();
        }
        m_workers.clear();
        m_queues.clear();
        ENGINE_LOG_ERROR("JobSystem: Failed to create worker threads: {}", error.what());
        return;
    }

    ENGINE_LOG_INFO("JobSystem initialized with {} worker threads.", threadCount);
}

void JobSystem::Shutdown() {
    if (t_workerIndex != 0) {
        ENGINE_LOG_ERROR("JobSystem: Shutdown must be requested from a non-worker thread.");
        return;
    }

    std::vector<std::thread> workers;
    {
        std::unique_lock<std::mutex> lock(m_lifecycleMutex);
        m_lifecycleChanged.wait(lock, [this] { return !m_shuttingDown; });
        if (!m_running.load(std::memory_order_acquire)) return;
        m_shuttingDown = true;
        m_running.store(false, std::memory_order_release);
        m_lifecycleChanged.wait(lock, [this] { return m_activeQueueUsers == 0; });
        workers.swap(m_workers);
    }

    // Workers continue draining queued jobs after the running flag is cleared.
    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }

    {
        std::lock_guard<std::mutex> lock(m_lifecycleMutex);
        m_queues.clear();
        m_shuttingDown = false;
    }
    m_lifecycleChanged.notify_all();
    ENGINE_LOG_INFO("JobSystem shut down after draining queued work.");
}

bool JobSystem::SubmitJob(JobFunction function, void* data, JobCounter* counter) {
    if (!function) return false;

    auto payload = std::make_unique<JobWrapperPayload>(JobWrapperPayload{function, data, counter});
    const Job job{JobWrapperCallback, payload.get()};
    if (!BeginQueueUse()) return false;

    while (true) {
        u32 queueIndex = 0;
        bool rejected = false;
        bool queued = false;
        {
            std::lock_guard<std::mutex> lock(m_lifecycleMutex);
            if (!m_running.load(std::memory_order_acquire) || m_queues.empty()) {
                rejected = true;
            } else {
                const u32 queueCount = static_cast<u32>(m_queues.size());
                queueIndex = t_workerIndex % queueCount;
                if (counter) counter->Increment();
                if (m_queues[queueIndex].Push(job)) {
                    payload.release();
                    queued = true;
                } else if (counter) {
                    counter->Decrement();
                }
            }
        }

        if (rejected || queued) {
            FinishQueueUse();
            return queued;
        }

        if (!ExecuteNextJob(queueIndex)) std::this_thread::yield();
    }
}

bool JobSystem::BeginQueueUse() noexcept {
    std::lock_guard<std::mutex> lock(m_lifecycleMutex);
    if (!m_running.load(std::memory_order_acquire) || m_queues.empty()) return false;
    ++m_activeQueueUsers;
    return true;
}

void JobSystem::FinishQueueUse() noexcept {
    {
        std::lock_guard<std::mutex> lock(m_lifecycleMutex);
        if (m_activeQueueUsers > 0) --m_activeQueueUsers;
    }
    m_lifecycleChanged.notify_all();
}

void JobSystem::ParallelFor(
    u32 totalCount,
    u32 batchSize,
    void (*function)(u32 index, void* data),
    void* data,
    JobCounter* counter
) {
    if (totalCount == 0 || !function) return;
    batchSize = std::max(1U, batchSize);

    const bool ownsQueueUse = BeginQueueUse();
    if (!ownsQueueUse) {
        for (u32 index = 0; index < totalCount; ++index) function(index, data);
        return;
    }

    const u32 numBatches = 1 + (totalCount - 1) / batchSize;
    for (u32 batch = 0; batch < numBatches; ++batch) {
        const u32 start = batch * batchSize;
        const u32 end = start + std::min(batchSize, totalCount - start);
        auto payload = std::make_unique<ParallelForPayload>(ParallelForPayload{function, data, start, end, counter});
        if (counter) counter->Increment();
        if (SubmitJob(ParallelForBatchCallback, payload.get())) {
            payload.release();
            continue;
        }
        if (counter) counter->Decrement();

        // If shutdown starts between batches, finish this batch synchronously.
        for (u32 index = start; index < end; ++index) function(index, data);
    }
    FinishQueueUse();
}

void JobSystem::WaitForCounter(JobCounter* counter) {
    if (!counter) return;
    const bool ownsQueueUse = BeginQueueUse();
    const bool workerMayDrain = ownsQueueUse || t_workerIndex != 0;
    while (!counter->IsComplete()) {
        if (!workerMayDrain || !ExecuteNextJob(t_workerIndex)) std::this_thread::yield();
    }
    if (ownsQueueUse) FinishQueueUse();
}

u32 JobSystem::GetWorkerCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_lifecycleMutex);
    return static_cast<u32>(m_workers.size());
}

void JobSystem::WorkerLoop(u32 workerIndex) {
    t_workerIndex = workerIndex;
    while (true) {
        if (ExecuteNextJob(workerIndex)) continue;
        if (!m_running.load(std::memory_order_acquire)) break;
        std::this_thread::yield();
    }
}

bool JobSystem::ExecuteNextJob(u32 workerIndex) {
    Job job;
    const u32 queueCount = static_cast<u32>(m_queues.size());
    if (queueCount == 0) return false;

    if (workerIndex < queueCount && m_queues[workerIndex].Pop(job)) {
        job.function(job.data);
        return true;
    }

    for (u32 offset = 1; offset < queueCount; ++offset) {
        const u32 victim = (workerIndex + offset) % queueCount;
        if (m_queues[victim].Steal(job)) {
            job.function(job.data);
            return true;
        }
    }
    return false;
}

} // namespace Engine::Core
