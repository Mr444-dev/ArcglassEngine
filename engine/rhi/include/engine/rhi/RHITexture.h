#pragma once

#include "engine/rhi/RHITypes.h"

namespace Engine::RHI {

class RHITexture {
public:
    virtual ~RHITexture() = default;

    [[nodiscard]] virtual Core::u32 GetWidth() const noexcept = 0;
    [[nodiscard]] virtual Core::u32 GetHeight() const noexcept = 0;
    [[nodiscard]] virtual Core::u32 GetDepth() const noexcept = 0;
    [[nodiscard]] virtual Core::u32 GetMipLevels() const noexcept = 0;
    [[nodiscard]] virtual Format GetFormat() const noexcept = 0;
    [[nodiscard]] virtual TextureUsage GetUsage() const noexcept = 0;

    [[nodiscard]] virtual ResourceState GetCurrentState() const noexcept = 0;
    virtual void SetCurrentState(ResourceState state) noexcept = 0;

    [[nodiscard]] virtual void* GetNativeResource() const noexcept = 0;
    [[nodiscard]] virtual Core::u32 GetBindlessIndex() const noexcept = 0;
    [[nodiscard]] virtual size_t GetRTVDescriptorHandle() const noexcept = 0;
    [[nodiscard]] virtual size_t GetDSVDescriptorHandle() const noexcept = 0;
};

} // namespace Engine::RHI
