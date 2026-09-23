#pragma once

#include "engine/rhi/RHITypes.h"

namespace Engine::RHI {

class RHIBuffer {
public:
    virtual ~RHIBuffer() = default;

    [[nodiscard]] virtual Core::u64 GetSizeInBytes() const noexcept = 0;
    [[nodiscard]] virtual BufferUsage GetUsage() const noexcept = 0;
    [[nodiscard]] virtual ResourceState GetCurrentState() const noexcept = 0;
    virtual void SetCurrentState(ResourceState state) noexcept = 0;

    [[nodiscard]] virtual void* Map() = 0;
    virtual void Unmap() = 0;

    [[nodiscard]] virtual void* GetNativeResource() const noexcept = 0;
    [[nodiscard]] virtual Core::u64 GetGPUVirtualAddress() const noexcept = 0;
    [[nodiscard]] virtual Core::u32 GetBindlessIndex() const noexcept = 0;
};

} // namespace Engine::RHI
