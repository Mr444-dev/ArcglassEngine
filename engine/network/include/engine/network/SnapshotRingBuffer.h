#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/Assert.h"
#include "engine/network/NetworkTypes.h"
#include <array>

namespace Engine::Network {

template <typename T, size_t Capacity = SNAPSHOT_BUFFER_CAPACITY>
class SnapshotRingBuffer {
public:
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two for fast bitwise indexing");

    SnapshotRingBuffer() {
        Clear();
    }

    void Clear() noexcept {
        m_valid.fill(false);
        m_latestTick = 0;
        m_hasData = false;
    }

    void Insert(Tick tick, const T& item) noexcept {
        const size_t slot = tick & (Capacity - 1);
        m_buffer[slot] = item;
        m_ticks[slot] = tick;
        m_valid[slot] = true;

        if (!m_hasData || tick > m_latestTick) {
            m_latestTick = tick;
            m_hasData = true;
        }
    }

    [[nodiscard]] bool TryGet(Tick tick, T& outItem) const noexcept {
        const size_t slot = tick & (Capacity - 1);
        if (m_valid[slot] && m_ticks[slot] == tick) {
            outItem = m_buffer[slot];
            return true;
        }
        return false;
    }

    [[nodiscard]] const T* Find(Tick tick) const noexcept {
        const size_t slot = tick & (Capacity - 1);
        if (m_valid[slot] && m_ticks[slot] == tick) {
            return &m_buffer[slot];
        }
        return nullptr;
    }

    [[nodiscard]] bool Contains(Tick tick) const noexcept {
        const size_t slot = tick & (Capacity - 1);
        return m_valid[slot] && m_ticks[slot] == tick;
    }

    [[nodiscard]] bool TryGetLatest(T& outItem) const noexcept {
        if (!m_hasData) return false;
        return TryGet(m_latestTick, outItem);
    }

    [[nodiscard]] Tick GetLatestTick() const noexcept {
        return m_latestTick;
    }

    [[nodiscard]] constexpr size_t GetCapacity() const noexcept {
        return Capacity;
    }

private:
    std::array<T, Capacity> m_buffer{};
    std::array<Tick, Capacity> m_ticks{};
    std::array<bool, Capacity> m_valid{};
    Tick m_latestTick{0};
    bool m_hasData{false};
};

} // namespace Engine::Network
