#pragma once

#include "engine/audio/AudioTypes.h"
#include "engine/world/Scene.h"
#include <vector>

namespace Engine::Audio {

class AudioEngine {
public:
    static AudioEngine& Get() noexcept;

    void Initialize();
    void Shutdown();

    void Update(float deltaTime, World::Scene& scene);

    void PlayOneShot(const AudioClip& clip, const Core::Vec3& worldPosition, float volume = 1.0f);

    [[nodiscard]] size_t GetActiveSourceCount() const noexcept { return m_activeSources; }

private:
    AudioEngine() = default;
    ~AudioEngine() = default;

    size_t m_activeSources{0};
    bool m_isInitialized{false};
};

} // namespace Engine::Audio
