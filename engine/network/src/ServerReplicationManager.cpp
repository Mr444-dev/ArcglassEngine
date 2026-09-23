#include "engine/network/ServerReplicationManager.h"
#include "engine/network/ClientPredictionSystem.h"
#include <algorithm>

namespace Engine::Network {

ServerReplicationManager::ServerReplicationManager() = default;

void ServerReplicationManager::RegisterEntity(Core::u32 entityId, const EntityPhysicalState& initialState) {
    m_entities[entityId] = initialState;
    m_historyBuffers[entityId].Clear();
    m_historyBuffers[entityId].Insert(initialState.tick, initialState);
}

ServerSnapshot ServerReplicationManager::StepEntity(Core::u32 entityId, const PlayerInput& input) noexcept {
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) {
        return {};
    }

    // Input Sanitization (Anti-Cheat clamping)
    PlayerInput cleanInput = input;
    cleanInput.moveX = std::clamp(input.moveX, -1.0f, 1.0f);
    cleanInput.moveZ = std::clamp(input.moveZ, -1.0f, 1.0f);

    EntityPhysicalState current = it->second;
    EntityPhysicalState next = ClientPredictionSystem::SimulatePhysicsStep(current, cleanInput, FIXED_DELTA_TIME);
    next.tick = current.tick + 1;

    it->second = next;
    m_historyBuffers[entityId].Insert(next.tick, next);

    if (next.tick > m_serverTick) {
        m_serverTick = next.tick;
    }

    ServerSnapshot snapshot;
    snapshot.tick = next.tick;
    snapshot.entityId = entityId;
    snapshot.state = next;
    return snapshot;
}

void ServerReplicationManager::ApplyExternalImpulse(Core::u32 entityId, const Core::Vec3& impulse) noexcept {
    auto it = m_entities.find(entityId);
    if (it != m_entities.end()) {
        it->second.position = it->second.position + impulse;
        m_historyBuffers[entityId].Insert(it->second.tick, it->second);
    }
}

bool ServerReplicationManager::TryGetEntityState(Core::u32 entityId, EntityPhysicalState& outState) const noexcept {
    auto it = m_entities.find(entityId);
    if (it != m_entities.end()) {
        outState = it->second;
        return true;
    }
    return false;
}

} // namespace Engine::Network
