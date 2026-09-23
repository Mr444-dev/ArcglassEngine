#include "engine/animation/Animation.h"
#include "engine/core/Log.h"

namespace Engine::Animation {

void Initialize() {
    ENGINE_LOG_INFO("Animation subsystem (ozz-animation) initialized.");
}

void Shutdown() {
    ENGINE_LOG_INFO("Animation subsystem shut down.");
}

} // namespace Engine::Animation
