#include "engine/physics/Physics.h"
#include "engine/core/Log.h"

namespace Engine::Physics {

void Initialize() {
    ENGINE_LOG_INFO("Physics subsystem (Jolt integration) initialized.");
}

void Shutdown() {
    ENGINE_LOG_INFO("Physics subsystem shut down.");
}

} // namespace Engine::Physics
