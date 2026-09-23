#include "engine/network/ClientPredictionSystem.h"
#include <cmath>

namespace Engine::Network {

ClientPredictionSystem::ClientPredictionSystem(Core::f32 reconcileThreshold)
    : m_reconcileThreshold(reconcileThreshold) {
    Reset({});
}

void ClientPredictionSystem::Reset(const EntityPhysicalState& initialState) noexcept {
    m_currentTick = initialState.tick;
    m_currentState = initialState;
    m_visualOffset = {0.0f, 0.0f, 0.0f};
    m_reconciliationCount = 0;

    m_inputHistory.Clear();
    m_stateHistory.Clear();
    m_stateHistory.Insert(m_currentTick, m_currentState);
}

EntityPhysicalState ClientPredictionSystem::PredictTick(const PlayerInput& input) noexcept {
    m_currentTick++;

    PlayerInput stampedInput = input;
    stampedInput.tick = m_currentTick;
    m_inputHistory.Insert(m_currentTick, stampedInput);

    m_currentState = SimulatePhysicsStep(m_currentState, stampedInput, FIXED_DELTA_TIME);
    m_stateHistory.Insert(m_currentTick, m_currentState);

    return m_currentState;
}

ReconcileResult ClientPredictionSystem::OnServerSnapshotReceived(const ServerSnapshot& snapshot) noexcept {
    ReconcileResult result;
    result.didReconcile = false;

    // Discard outdated snapshots older than our buffer window
    if (snapshot.tick > m_currentTick) {
        // Snapshot is in the future - snap directly
        m_currentTick = snapshot.tick;
        m_currentState = snapshot.state;
        m_stateHistory.Insert(m_currentTick, m_currentState);
        return result;
    }

    EntityPhysicalState predictedAtTick;
    if (!m_stateHistory.TryGet(snapshot.tick, predictedAtTick)) {
        // State not found in history (snapshot too old), snap to avoid permanent drift
        m_currentState = snapshot.state;
        m_currentTick = snapshot.tick;
        m_stateHistory.Insert(m_currentTick, m_currentState);
        return result;
    }

    const Core::Vec3 diff = predictedAtTick.position - snapshot.state.position;
    const Core::f32 error = diff.Length();
    result.errorMagnitude = error;

    if (error > m_reconcileThreshold) {
        result.didReconcile = true;
        result.originalPredictedPos = m_currentState.position;

        // 1. Rollback to server authoritative state at snapshot.tick
        EntityPhysicalState replayState = snapshot.state;
        m_stateHistory.Insert(snapshot.tick, replayState);

        Core::u32 replayedCount = 0;

        // 2. Replay all subsequent stored inputs up to current client tick
        for (Tick t = snapshot.tick + 1; t <= m_currentTick; ++t) {
            PlayerInput storedInput;
            if (m_inputHistory.TryGet(t, storedInput)) {
                replayState = SimulatePhysicsStep(replayState, storedInput, FIXED_DELTA_TIME);
                m_stateHistory.Insert(t, replayState);
                replayedCount++;
            }
        }

        result.replayedTicksCount = replayedCount;
        result.reconciledPos = replayState.position;

        // 3. Compute visual smoothing offset (decays to zero over time)
        // Offset = old visual position - new physics position
        m_visualOffset = m_visualOffset + (result.originalPredictedPos - result.reconciledPos);

        // 4. Update current physical state
        m_currentState = replayState;
        m_reconciliationCount++;
    }

    return result;
}

void ClientPredictionSystem::UpdateVisualSmoothing(Core::f32 deltaTime) noexcept {
    if (m_visualOffset.Dot(m_visualOffset) > 0.000001f) {
        // Exponential decay towards zero: offset * exp(-decay * dt)
        const Core::f32 factor = std::exp(-m_smoothingDecayRate * deltaTime);
        m_visualOffset = m_visualOffset * factor;

        if (m_visualOffset.Dot(m_visualOffset) < 0.000001f) {
            m_visualOffset = {0.0f, 0.0f, 0.0f};
        }
    }
}

Core::Vec3 ClientPredictionSystem::GetRenderPosition() const noexcept {
    return m_currentState.position + m_visualOffset;
}

EntityPhysicalState ClientPredictionSystem::SimulatePhysicsStep(
    const EntityPhysicalState& fromState,
    const PlayerInput& input,
    Core::f32 dt) noexcept {
    EntityPhysicalState next = fromState;
    next.tick = fromState.tick + 1;

    constexpr Core::f32 walkSpeed = 6.0f;
    constexpr Core::f32 sprintMultiplier = 1.5f;
    constexpr Core::f32 gravity = -9.81f;
    constexpr Core::f32 jumpVelocity = 5.0f;

    const Core::f32 currentSpeed = input.sprint ? (walkSpeed * sprintMultiplier) : walkSpeed;

    next.velocity.x = input.moveX * currentSpeed;
    next.velocity.z = input.moveZ * currentSpeed;

    if (input.jump && next.isGrounded) {
        next.velocity.y = jumpVelocity;
        next.isGrounded = false;
    } else if (!next.isGrounded) {
        next.velocity.y += gravity * dt;
    }

    next.position = next.position + (next.velocity * dt);

    if (next.position.y <= 0.0f) {
        next.position.y = 0.0f;
        next.velocity.y = 0.0f;
        next.isGrounded = true;
    }

    return next;
}

} // namespace Engine::Network
