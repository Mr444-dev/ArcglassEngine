#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"

namespace Engine::Audio {

class SpatialAudio {
public:
    static float CalculateAttenuation(float distance, float minDistance, float maxDistance) noexcept;

    static void CalculateStereoPan(
        const Core::Vec3& sourcePos,
        const Core::Vec3& listenerPos,
        const Core::Vec3& listenerForward,
        const Core::Vec3& listenerUp,
        float& outLeftGain,
        float& outRightGain
    ) noexcept;
};

} // namespace Engine::Audio
