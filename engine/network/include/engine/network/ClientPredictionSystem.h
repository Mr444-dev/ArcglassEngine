#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/network/NetworkTypes.h"
#include "engine/network/SnapshotRingBuffer.h"

namespace Engine::Network {

class ClientPredictionSystem {
public:
    explicit ClientPredictionSystem(Core::f32 reconcileThreshold = DEFAULT_RECONCILE_THRESHOLD);

    void Reset(const EntityPhysicalState& initialState) noexcept;

    // Advances client prediction by one fixed tick using the provided input
    EntityPhysicalState PredictTick(const PlayerInput& input) noexcept;

    // Called when authoritative snapshot arrives from server
    ReconcileResult OnServerSnapshotReceived(const ServerSnapshot& snapshot) noexcept;

    // Updates visual offset decay for smooth rendering without snapping
    void UpdateVisualSmoothing(Core::f32 deltaTime) noexcept;

    // Returns physics position plus visual smoothing offset
    [[nodiscard]] Core::Vec3 GetRenderPosition() const noexcept;

    [[nodiscard]] const EntityPhysicalState& GetCurrentState() const noexcept { return m_currentState; }
    [[nodiscard]] Tick GetCurrentTick() const noexcept { return m_currentTick; }
    [[nodiscard]] const Core::Vec3& GetVisualOffset() const noexcept { return m_visualOffset; }
    [[nodiscard]] Core::u32 GetReconciliationCount() const noexcept { return m_reconciliationCount; }

    static EntityPhysicalState SimulatePhysicsStep(
        const EntityPhysicalState& fromState,
        const PlayerInput& input,
        Core::f32 dt) noexcept;

private:
    Tick m_currentTick{0};
    EntityPhysicalState m_currentState{};
    SnapshotRingBuffer<PlayerInput> m_inputHistory;
    SnapshotRingBuffer<EntityPhysicalState> m_stateHistory;

    Core::f32 m_reconcileThreshold;
    Core::Vec3 m_visualOffset{0.0f, 0.0f, 0.0f};
    Core::f32 m_smoothingDecayRate{15.0f}; // Exponential decay factor (1/s)
    Core::u32 m_reconciliationCount{0};
};

} // namespace Engine::Network
