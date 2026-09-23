#pragma once

#include "engine/core/BaseTypes.h"

namespace Engine::RHI {

enum class GraphicsAPI : Core::u8 {
    DirectX12,
    Vulkan
};

void Initialize(GraphicsAPI api);
void Shutdown();

} // namespace Engine::RHI
