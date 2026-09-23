#include "engine/core/JobSystem.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/core/Profiler.h"
#include <algorithm>
#include <chrono>

namespace Engine::Core {

namespace {
thread_local u32 t_workerIndex = 0;

struct JobWrapperPayload {
    JobFunction originalFunction{nullptr};
    void* originalData{nullptr};
    JobCounter* counter{nullptr};
};

void JobWrapperCallback(void* rawData) {
    auto* payload = static_cast<JobWrapperPayload*>(rawData);
    if (payload->originalFunction) {
        payload->originalFunction(payload->originalData);
    }
    if (payload->counter) {
        payload->counter->Decrement();
    }
    delete payload;
}

struct ParallelForPayload {
    void (*function)(u32, void*);
    void* data;
    u32 startIndex;
    u32 endIndex;
    JobCounter* counter;
};

void ParallelForBatchCallback(void* rawData) {
    auto* payload = static_cast<ParallelForPayload*>(rawData);
    for (u32 i = payload->startIndex; i < payload->endIndex; ++i) {
        payload->function(i, payload->data);
    }
    if (payload->counter) {
        payload->counter->Decrement();
    }
    delete payload;
}
} // namespace

// WorkStealingQueue implementation
bool WorkStealingQueue::Push(const Job& job) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_tail - m_head >= CAPACITY) {
        return false;
    }
    m_buffer[m_tail & MASK] = job;
    m_tail++;
    return true;
}

bool WorkStealingQueue::Pop(Job& outJob) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_tail <= m_head) {
        return false;
    }
    m_tail--;
    outJob = m_buffer[m_tail & MASK];
    return true;
}

bool WorkStealingQueue::Steal(Job& outJob) {
    std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
    if (!lock.owns_lock() || m_tail <= m_head) {
        return false;
    }
    outJob = m_buffer[m_head & MASK];
    m_head++;
    return true;
}

bool WorkStealingQueue::IsEmpty() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tail <= m_head;
}

// JobSystem implementation
JobSystem& JobSystem::Get() {
    static JobSystem instance;
    return instance;
}

JobSystem::~JobSystem() {
    Shutdown();
}

void JobSystem::Initialize(u32 threadCount) {
    if (m_running.load(std::memory_order_acquire)) {
        return;
    }

    if (threadCount == 0) {
        const u32 hardwareThreads = std::thread::hardware_concurrency();
        threadCount = (hardwareThreads > 1) ? (hardwareThreads - 1) : 1;
    }

    m_running.store(true, std::memory_order_release);
    m_queues = std::vector<WorkStealingQueue>(threadCount + 1); // +1 for main thread

    m_workers.reserve(threadCount);
    for (u32 i = 0; i < threadCount; ++i) {
        m_workers.emplace_back(&JobSystem::WorkerLoop, this, i + 1);
    }

    ENGINE_LOG_INFO("JobSystem initialized with {} worker threads.", threadCount);
}

void JobSystem::Shutdown() {
    if (!m_running.load(std::memory_order_acquire)) {
        return;
    }

    m_running.store(false, std::memory_order_release);

    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();
    m_queues.clear();

    ENGINE_LOG_INFO("JobSystem shut down cleanly.");
}

void JobSystem::SubmitJob(JobFunction function, void* data, JobCounter* counter) {
    if (!m_running.load(std::memory_order_acquire)) {
        return;
    }

    if (counter) {
        counter->Increment();
    }

    auto* payload = new JobWrapperPayload{function, data, counter};
    Job job{JobWrapperCallback, payload};

    const u32 queueIdx = t_workerIndex % static_cast<u32>(m_queues.size());
    while (!m_queues[queueIdx].Push(job)) {
        ExecuteNextJob(queueIdx);
    }
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

    const u32 numBatches = (totalCount + batchSize - 1) / batchSize;
    if (counter) {
        counter->Increment(numBatches);
    }

    const u32 queueIdx = t_workerIndex % static_cast<u32>(m_queues.size());
    for (u32 b = 0; b < numBatches; ++b) {
        const u32 start = b * batchSize;
        const u32 end = std::min(start + batchSize, totalCount);

        auto* payload = new ParallelForPayload{function, data, start, end, counter};
        Job job{ParallelForBatchCallback, payload};

        while (!m_queues[queueIdx].Push(job)) {
            ExecuteNextJob(queueIdx);
        }
    }
}

void JobSystem::WaitForCounter(JobCounter* counter) {
    if (!counter) return;

    while (!counter->IsComplete()) {
        if (!ExecuteNextJob(t_workerIndex)) {
            std::this_thread::yield();
        }
    }
}

void JobSystem::WorkerLoop(u32 workerIndex) {
    t_workerIndex = workerIndex;

    while (m_running.load(std::memory_order_acquire)) {
        if (!ExecuteNextJob(workerIndex)) {
            std::this_thread::yield();
        }
    }
}

bool JobSystem::ExecuteNextJob(u32 workerIndex) {
    Job job;

    // 1. Try to pop from own queue (LIFO)
    if (workerIndex < m_queues.size() && m_queues[workerIndex].Pop(job)) {
        job.function(job.data);
        return true;
    }

    // 2. Steal from other queues (FIFO)
    const u32 queueCount = static_cast<u32>(m_queues.size());
    for (u32 i = 0; i < queueCount; ++i) {
        const u32 victim = (workerIndex + i + 1) % queueCount;
        if (m_queues[victim].Steal(job)) {
            job.function(job.data);
            return true;
        }
    }

    return false;
}

} // namespace Engine::Core
