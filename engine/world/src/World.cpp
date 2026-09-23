#include "engine/world/World.h"
#include "engine/core/Log.h"

namespace Engine::World {

void Initialize() {
    ENGINE_LOG_INFO("World & Scene subsystem initialized.");
}

void Shutdown() {
    ENGINE_LOG_INFO("World & Scene subsystem shut down.");
}

} // namespace Engine::World
