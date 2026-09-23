#include "engine/renderer/Renderer.h"
#include "engine/core/Log.h"

namespace Engine::Renderer {

void Initialize() {
    ENGINE_LOG_INFO("High-level Renderer initialized.");
}

void Shutdown() {
    ENGINE_LOG_INFO("High-level Renderer shut down.");
}

} // namespace Engine::Renderer
