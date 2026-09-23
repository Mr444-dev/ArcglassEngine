#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/network/NetworkTypes.h"
#include "engine/network/SnapshotRingBuffer.h"
#include "engine/network/ClientPredictionSystem.h"
#include "engine/network/ServerReplicationManager.h"

void TestSnapshotRingBuffer() {
    using namespace Engine::Core;
    using namespace Engine::Network;

    Logger::LogMessage(LogLevel::Info, "--- Testing SnapshotRingBuffer (Fixed Ring Buffer, Zero Alloc) ---");

    SnapshotRingBuffer<EntityPhysicalState, 64> buffer;
    ENGINE_ASSERT(buffer.GetCapacity() == 64, "Capacity mismatch");

    // Insert 100 entries (will wrap around slot indexing)
    for (Tick t = 1; t <= 100; ++t) {
        EntityPhysicalState s;
        s.tick = t;
        s.position = {static_cast<f32>(t), 0.0f, 0.0f};
        buffer.Insert(t, s);
    }

    ENGINE_ASSERT(buffer.GetLatestTick() == 100, "Latest tick mismatch");

    // Recent tick 100 should be present
    EntityPhysicalState retrieved;
    ENGINE_ASSERT(buffer.TryGet(100, retrieved), "Tick 100 should exist");
    ENGINE_ASSERT(retrieved.position.x == 100.0f, "Position mismatch");

    // Tick 90 should be present
    ENGINE_ASSERT(buffer.TryGet(90, retrieved), "Tick 90 should exist");
    ENGINE_ASSERT(retrieved.position.x == 90.0f, "Position mismatch");

    // Very old tick 10 should have been overwritten (since capacity is 64)
    ENGINE_ASSERT(!buffer.TryGet(10, retrieved), "Tick 10 must be overwritten by circular buffer");

    ENGINE_LOG_INFO("SnapshotRingBuffer wraparound and lookup verified (Capacity: {}).", buffer.GetCapacity());
}

void TestClientPredictionLoop() {
    using namespace Engine::Core;
    using namespace Engine::Network;

    Logger::LogMessage(LogLevel::Info, "--- Testing Client-Side Prediction Simulation Loop ---");

    ClientPredictionSystem client(0.02f);
    EntityPhysicalState initial;
    initial.tick = 0;
    initial.position = {0.0f, 0.0f, 0.0f};
    client.Reset(initial);

    // Predict 60 ticks of forward sprint (60 * (1/60s) = 1.0 second)
    // Speed: 6.0 m/s * 1.5 (sprint) = 9.0 m/s
    PlayerInput input;
    input.moveZ = 1.0f;
    input.sprint = true;

    for (u32 i = 0; i < 60; ++i) {
        client.PredictTick(input);
    }

    ENGINE_ASSERT(client.GetCurrentTick() == 60, "Current tick mismatch");
    const Vec3 finalPos = client.GetCurrentState().position;

    // Expected distance: ~9.0 m in Z axis
    ENGINE_ASSERT(std::abs(finalPos.z - 9.0f) < 0.01f, "Final predicted Z position incorrect");
    ENGINE_ASSERT(finalPos.x == 0.0f && finalPos.y == 0.0f, "X/Y should remain 0");

    ENGINE_LOG_INFO("Client prediction simulated 60 ticks deterministically (Final Z: {:.2f} m).", finalPos.z);
}

void TestServerReconciliationOnDesync() {
    using namespace Engine::Core;
    using namespace Engine::Network;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 15: Server Reconciliation & Rollback Replay ---");

    ClientPredictionSystem client(0.02f);
    ServerReplicationManager server;

    constexpr u32 entityId = 42;
    EntityPhysicalState initial;
    initial.tick = 0;
    initial.position = {0.0f, 0.0f, 0.0f};

    client.Reset(initial);
    server.RegisterEntity(entityId, initial);

    PlayerInput input;
    input.moveZ = 1.0f; // Walk forward at 6.0 m/s

    // 1. Client predicts ahead: ticks 1 to 20
    for (u32 t = 1; t <= 20; ++t) {
        client.PredictTick(input);
    }

    const Vec3 clientPredictedAt20 = client.GetCurrentState().position;
    // Expected distance at 20 ticks: 20 * (1/60) * 6.0 = 2.0 m
    ENGINE_ASSERT(std::abs(clientPredictedAt20.z - 2.0f) < 0.01f, "Prediction distance mismatch");

    // 2. Server simulates ticks 1 to 10
    ServerSnapshot snapshotAt10;
    for (u32 t = 1; t <= 10; ++t) {
        input.tick = t;
        snapshotAt10 = server.StepEntity(entityId, input);
    }

    // 3. Simulate an unexpected server-side impulse (e.g. explosion/wind push +1.5m on X) at tick 10
    server.ApplyExternalImpulse(entityId, {1.5f, 0.0f, 0.0f});
    server.TryGetEntityState(entityId, snapshotAt10.state);

    // 4. Server snapshot for tick 10 arrives at client (client is currently at tick 20)
    ReconcileResult reconcile = client.OnServerSnapshotReceived(snapshotAt10);

    ENGINE_ASSERT(reconcile.didReconcile, "Client must detect deviation and trigger rollback");
    ENGINE_ASSERT(reconcile.replayedTicksCount == 10, "Client must replay ticks 11..20 (10 ticks)");
    ENGINE_ASSERT(reconcile.errorMagnitude >= 1.49f, "Error magnitude should match impulse");

    // 5. After reconciliation, client's physics position at tick 20 should now incorporate the +1.5m X shift
    const Vec3 reconciledPos = client.GetCurrentState().position;
    ENGINE_ASSERT(std::abs(reconciledPos.x - 1.5f) < 0.01f, "Reconciled X position must reflect server impulse");
    ENGINE_ASSERT(std::abs(reconciledPos.z - 2.0f) < 0.01f, "Reconciled Z position must retain forward movement");

    // 6. Visual offset must temporarily counteract the snap
    const Vec3 visualOffset = client.GetVisualOffset();
    ENGINE_ASSERT(visualOffset.Length() > 0.5f, "Visual smoothing offset must be non-zero");

    ENGINE_LOG_INFO("Reconciliation successfully rolled back to tick 10, replayed 10 ticks, and eliminated desync (Reconciled: [{:.2f}, {:.2f}, {:.2f}]).",
        reconciledPos.x, reconciledPos.y, reconciledPos.z);
}

void TestVisualSmoothingOffsetDecay() {
    using namespace Engine::Core;
    using namespace Engine::Network;

    Logger::LogMessage(LogLevel::Info, "--- Testing Visual Smoothing Offset Exponential Decay ---");

    ClientPredictionSystem client(0.02f);
    EntityPhysicalState initial;
    client.Reset(initial);

    // Predict 5 ticks
    PlayerInput input;
    input.moveX = 1.0f;
    for (u32 i = 0; i < 5; ++i) client.PredictTick(input);

    // Induce reconciliation
    ServerSnapshot snap;
    snap.tick = 5;
    snap.state.tick = 5;
    snap.state.position = {0.0f, 0.0f, 0.0f}; // Forced mismatch
    (void)client.OnServerSnapshotReceived(snap);

    f32 initialOffsetLen = client.GetVisualOffset().Length();
    ENGINE_ASSERT(initialOffsetLen > 0.1f, "Initial offset must be non-zero");

    // Simulate 10 rendering frames with dt = 0.016s (~60 FPS)
    for (int frame = 0; frame < 10; ++frame) {
        client.UpdateVisualSmoothing(0.016f);
    }

    f32 decayedOffsetLen = client.GetVisualOffset().Length(); (void)decayedOffsetLen;
    ENGINE_ASSERT(decayedOffsetLen < initialOffsetLen, "Offset must decay over time");

    // Simulate another 30 frames (total ~0.64s)
    for (int frame = 0; frame < 30; ++frame) {
        client.UpdateVisualSmoothing(0.016f);
    }

    ENGINE_ASSERT(client.GetVisualOffset().Length() < 0.001f, "Offset must fully decay to zero");
    ENGINE_LOG_INFO("Visual offset decayed smoothly from {:.3f} m to {:.5f} m without visual snapping.",
        initialOffsetLen, client.GetVisualOffset().Length());
}

void RunNetworkRollbackTests() {
    TestSnapshotRingBuffer();
    TestClientPredictionLoop();
    TestServerReconciliationOnDesync();
    TestVisualSmoothingOffsetDecay();
}
