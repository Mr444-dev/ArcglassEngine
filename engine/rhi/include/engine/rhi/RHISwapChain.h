#pragma once

#include "engine/rhi/RHITexture.h"

namespace Engine::RHI {

class RHISwapChain {
public:
    static constexpr Core::u32 BUFFER_COUNT = 3; // Triple buffering standard

    virtual ~RHISwapChain() = default;

    virtual void Present(bool vsync = true) = 0;
    virtual void Resize(Core::u32 width, Core::u32 height) = 0;

    [[nodiscard]] virtual RHITexture* GetCurrentBackBuffer() const noexcept = 0;
    [[nodiscard]] virtual Core::u32 GetCurrentBackBufferIndex() const noexcept = 0;
    [[nodiscard]] virtual Core::u32 GetWidth() const noexcept = 0;
    [[nodiscard]] virtual Core::u32 GetHeight() const noexcept = 0;
};

} // namespace Engine::RHI
