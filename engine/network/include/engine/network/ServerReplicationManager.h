#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/network/NetworkTypes.h"
#include "engine/network/SnapshotRingBuffer.h"
#include <unordered_map>

namespace Engine::Network {

class ServerReplicationManager {
public:
    ServerReplicationManager();

    void RegisterEntity(Core::u32 entityId, const EntityPhysicalState& initialState);
    
    // Executes an authoritative simulation tick on the server using client input
    ServerSnapshot StepEntity(Core::u32 entityId, const PlayerInput& input) noexcept;

    // Injects an authoritative external force or position shift (e.g. explosion, wind)
    void ApplyExternalImpulse(Core::u32 entityId, const Core::Vec3& impulse) noexcept;

    [[nodiscard]] bool TryGetEntityState(Core::u32 entityId, EntityPhysicalState& outState) const noexcept;
    [[nodiscard]] Tick GetServerTick() const noexcept { return m_serverTick; }

private:
    Tick m_serverTick{0};
    std::unordered_map<Core::u32, EntityPhysicalState> m_entities;
    std::unordered_map<Core::u32, SnapshotRingBuffer<EntityPhysicalState>> m_historyBuffers;
};

} // namespace Engine::Network
