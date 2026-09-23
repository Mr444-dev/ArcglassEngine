#include "engine/platform/Platform.h"
#include "engine/core/Log.h"

namespace Engine::Platform {

void Initialize() {
    ENGINE_LOG_INFO("Platform subsystem initialized.");
}

void Shutdown() {
    ENGINE_LOG_INFO("Platform subsystem shut down.");
}

} // namespace Engine::Platform
