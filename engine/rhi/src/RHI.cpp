#include "engine/rhi/RHI.h"
#include "engine/core/Log.h"

namespace Engine::RHI {

void Initialize(GraphicsAPI api) {
    const char* apiName = (api == GraphicsAPI::DirectX12) ? "DirectX 12 Agility SDK" : "Vulkan 1.3/1.4";
    ENGINE_LOG_INFO("RHI initialized with backend: {}", apiName);
}

void Shutdown() {
    ENGINE_LOG_INFO("RHI shut down.");
}

} // namespace Engine::RHI
