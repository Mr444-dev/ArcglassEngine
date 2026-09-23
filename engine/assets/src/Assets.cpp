#include "engine/assets/Assets.h"
#include "engine/core/Log.h"

namespace Engine::Assets {

void Initialize() {
    ENGINE_LOG_INFO("Asset pipeline & VFS initialized.");
}

void Shutdown() {
    ENGINE_LOG_INFO("Asset pipeline & VFS shut down.");
}

} // namespace Engine::Assets
