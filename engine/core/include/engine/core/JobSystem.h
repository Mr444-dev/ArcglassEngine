#pragma once

#include "engine/core/BaseTypes.h"
#include <atomic>
#include <vector>
#include <thread>
#include <mutex>

namespace Engine::Core {

using JobFunction = void (*)(void* data);

struct Job {
    JobFunction function{nullptr};
    void* data{nullptr};
};

// Counter tracking unfinished jobs in a task dependency group
class JobCounter {
public:
    JobCounter() : m_count(0) {}
    explicit JobCounter(u32 initialCount) : m_count(initialCount) {}

    void Increment(u32 count = 1) noexcept {
        m_count.fetch_add(count, std::memory_order_relaxed);
    }

    void Decrement() noexcept {
        m_count.fetch_sub(1, std::memory_order_release);
    }

    [[nodiscard]] bool IsComplete() const noexcept {
        return m_count.load(std::memory_order_acquire) == 0;
    }

    [[nodiscard]] u32 GetValue() const noexcept {
        return m_count.load(std::memory_order_relaxed);
    }

private:
    std::atomic<u32> m_count;
};

// Work-Stealing Ring Buffer Deque (LIFO for owner, FIFO try_to_lock for thieves)
class WorkStealingQueue {
public:
    static constexpr size_t CAPACITY = 65536;
    static constexpr size_t MASK = CAPACITY - 1;

    WorkStealingQueue() : m_head(0), m_tail(0) {}
    ~WorkStealingQueue() = default;

    bool Push(const Job& job);
    bool Pop(Job& outJob);
    bool Steal(Job& outJob);

    [[nodiscard]] bool IsEmpty() const noexcept;

private:
    mutable std::mutex m_mutex;
    size_t m_head{0};
    size_t m_tail{0};
    Job m_buffer[CAPACITY];
};

// Main Multithreaded Job System Scheduler
class JobSystem {
public:
    static JobSystem& Get();

    void Initialize(u32 threadCount = 0);
    void Shutdown();

    // Submit single job
    void SubmitJob(JobFunction function, void* data = nullptr, JobCounter* counter = nullptr);

    // Parallel For split across threads
    void ParallelFor(
        u32 totalCount,
        u32 batchSize,
        void (*function)(u32 index, void* data),
        void* data = nullptr,
        JobCounter* counter = nullptr
    );

    // Wait on a counter; worker thread steals jobs while waiting
    void WaitForCounter(JobCounter* counter);

    [[nodiscard]] u32 GetWorkerCount() const noexcept { return static_cast<u32>(m_workers.size()); }
    [[nodiscard]] bool IsRunning() const noexcept { return m_running.load(std::memory_order_acquire); }

private:
    JobSystem() = default;
    ~JobSystem();

    void WorkerLoop(u32 workerIndex);
    bool ExecuteNextJob(u32 workerIndex);

    std::vector<std::thread> m_workers;
    std::vector<WorkStealingQueue> m_queues;
    std::atomic<bool> m_running{false};
};

} // namespace Engine::Core
