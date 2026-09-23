#include "engine/audio/SpatialAudio.h"
#include <cmath>
#include <algorithm>

namespace Engine::Audio {

float SpatialAudio::CalculateAttenuation(float distance, float minDistance, float maxDistance) noexcept {
    if (distance <= minDistance) {
        return 1.0f;
    }
    if (distance >= maxDistance || maxDistance <= minDistance) {
        return 0.0f;
    }

    // Inverse distance rolloff with linear cutoff taper
    const float d = distance - minDistance;
    const float maxD = maxDistance - minDistance;
    const float inverseFactor = minDistance / (minDistance + d);
    const float linearCutoff = 1.0f - (d / maxD);

    return std::clamp(inverseFactor * linearCutoff, 0.0f, 1.0f);
}

void SpatialAudio::CalculateStereoPan(
    const Core::Vec3& sourcePos,
    const Core::Vec3& listenerPos,
    const Core::Vec3& listenerForward,
    const Core::Vec3& listenerUp,
    float& outLeftGain,
    float& outRightGain
) noexcept {
    const Core::Vec3 toSource = sourcePos - listenerPos;
    const float dist = toSource.Length();

    if (dist < 1e-4f) {
        outLeftGain = 0.7071f;
        outRightGain = 0.7071f;
        return;
    }

    const Core::Vec3 dir = toSource * (1.0f / dist);
    const Core::Vec3 right = listenerUp.Cross(listenerForward).Normalized();

    // Dot with right vector: +1.0 = fully on listener's right, -1.0 = fully on listener's left
    const float pan = std::clamp(dir.Dot(right), -1.0f, 1.0f);

    // Constant power panning law
    outLeftGain  = std::sqrt(0.5f * (1.0f - pan));
    outRightGain = std::sqrt(0.5f * (1.0f + pan));
}

} // namespace Engine::Audio
