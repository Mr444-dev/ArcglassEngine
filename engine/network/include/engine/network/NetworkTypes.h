#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"

namespace Engine::Network {

using Tick = Core::u64;

inline constexpr Core::u32 FIXED_TICK_RATE = 60;
inline constexpr Core::f32 FIXED_DELTA_TIME = 1.0f / static_cast<Core::f32>(FIXED_TICK_RATE);
inline constexpr size_t SNAPSHOT_BUFFER_CAPACITY = 256;
inline constexpr Core::f32 DEFAULT_RECONCILE_THRESHOLD = 0.02f; // 2 cm error tolerance

struct PlayerInput {
    Tick tick{0};
    Core::f32 moveX{0.0f};      // Horizontal axis [-1.0 .. 1.0]
    Core::f32 moveZ{0.0f};      // Forward/backward axis [-1.0 .. 1.0]
    bool jump{false};
    bool sprint{false};

    [[nodiscard]] constexpr bool HasMovement() const noexcept {
        return moveX != 0.0f || moveZ != 0.0f || jump;
    }
};

struct EntityPhysicalState {
    Tick tick{0};
    Core::Vec3 position{0.0f, 0.0f, 0.0f};
    Core::Vec3 velocity{0.0f, 0.0f, 0.0f};
    Core::f32 yaw{0.0f};
    bool isGrounded{true};

    [[nodiscard]] Core::f32 DistanceTo(const EntityPhysicalState& other) const noexcept {
        return (position - other.position).Length();
    }
};

struct ReconcileResult {
    bool didReconcile{false};
    Core::f32 errorMagnitude{0.0f};
    Core::u32 replayedTicksCount{0};
    Core::Vec3 originalPredictedPos{0.0f, 0.0f, 0.0f};
    Core::Vec3 reconciledPos{0.0f, 0.0f, 0.0f};
};

struct ServerSnapshot {
    Tick tick{0};
    Core::u32 entityId{0};
    EntityPhysicalState state;
};

} // namespace Engine::Network
