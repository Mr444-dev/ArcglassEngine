#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/MathTypes.h"
#include <string>
#include <vector>

namespace Engine::Audio {

struct AudioClip {
    std::string name;
    Core::u32 sampleRate{44100};
    Core::u16 channels{2};
    Core::u16 bitDepth{16};
    float durationSeconds{1.0f};
    std::vector<float> pcmSamples; // Normalized [-1.0f, 1.0f]
};

struct AudioSourceComponent {
    const AudioClip* clip{nullptr};
    float volume{1.0f};
    float pitch{1.0f};
    bool loop{false};
    bool isPlaying{false};

    float minDistance{1.0f};
    float maxDistance{50.0f};
    float spatialBlend{1.0f}; // 0 = 2D Non-diegetic, 1 = 3D Spatial
    float playbackTime{0.0f};

    // Evaluated per-frame stereo gains
    float currentLeftGain{1.0f};
    float currentRightGain{1.0f};
};

struct AudioListenerComponent {
    Core::Vec3 position{0.0f, 0.0f, 0.0f};
    Core::Vec3 forward{0.0f, 0.0f, 1.0f};
    Core::Vec3 up{0.0f, 1.0f, 0.0f};
    bool isPrimary{true};
};

} // namespace Engine::Audio
