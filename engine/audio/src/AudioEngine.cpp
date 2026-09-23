#include "engine/audio/AudioEngine.h"
#include "engine/audio/SpatialAudio.h"
#include "engine/world/Components.h"
#include "engine/core/Log.h"

namespace Engine::Audio {

AudioEngine& AudioEngine::Get() noexcept {
    static AudioEngine instance;
    return instance;
}

void AudioEngine::Initialize() {
    m_isInitialized = true;
    ENGINE_LOG_INFO("AudioEngine initialized (Spatial 3D Audio enabled).");
}

void AudioEngine::Shutdown() {
    m_isInitialized = false;
    ENGINE_LOG_INFO("AudioEngine shutdown.");
}

void AudioEngine::PlayOneShot(const AudioClip& clip, const Core::Vec3& worldPosition, float volume) {
    (void)clip;
    (void)worldPosition;
    (void)volume;
    ENGINE_LOG_INFO("AudioEngine: OneShot sound triggered at ({:.2f}, {:.2f}, {:.2f}).",
        worldPosition.x, worldPosition.y, worldPosition.z);
}

void AudioEngine::Update(float deltaTime, World::Scene& scene) {
    if (!m_isInitialized) return;

    auto& registry = scene.GetRegistry();
    m_activeSources = 0;

    // 1. Locate primary listener
    AudioListenerComponent listener{};
    listener.position = {0.0f, 0.0f, 0.0f};
    listener.forward  = {0.0f, 0.0f, 1.0f};
    listener.up       = {0.0f, 1.0f, 0.0f};

    if (registry.HasPool<AudioListenerComponent>()) {
        const auto& listenerPool = registry.GetPool<AudioListenerComponent>();
        for (World::EntityID e : listenerPool.GetEntities()) {
            const auto& l = listenerPool.Get(e);
            if (l.isPrimary) {
                listener = l;
                // If listener has transform, sync position
                if (registry.Has<World::TransformComponent>(e)) {
                    listener.position = registry.Get<World::TransformComponent>(e).position;
                }
                break;
            }
        }
    }

    // 2. Update active audio sources
    if (registry.HasPool<AudioSourceComponent>()) {
        auto& sourcePool = registry.GetPool<AudioSourceComponent>();
        for (World::EntityID e : sourcePool.GetEntities()) {
            auto& src = sourcePool.Get(e);
            if (!src.isPlaying) continue;

            m_activeSources++;
            src.playbackTime += deltaTime * src.pitch;

            Core::Vec3 srcPos{0.0f, 0.0f, 0.0f};
            if (registry.Has<World::TransformComponent>(e)) {
                srcPos = registry.Get<World::TransformComponent>(e).position;
            }

            const float dist = (srcPos - listener.position).Length();
            const float atten = SpatialAudio::CalculateAttenuation(dist, src.minDistance, src.maxDistance);

            float panL = 0.7071f;
            float panR = 0.7071f;
            SpatialAudio::CalculateStereoPan(srcPos, listener.position, listener.forward, listener.up, panL, panR);

            // Blend 2D vs 3D
            src.currentLeftGain  = src.volume * (atten * panL * src.spatialBlend + 0.7071f * (1.0f - src.spatialBlend));
            src.currentRightGain = src.volume * (atten * panR * src.spatialBlend + 0.7071f * (1.0f - src.spatialBlend));
        }
    }
}

} // namespace Engine::Audio
