#pragma once

#include "engine/core/BaseTypes.h"

namespace Engine::RHI {

class RHIFence {
public:
    virtual ~RHIFence() = default;

    virtual void Signal(Core::u64 value) = 0;
    virtual void WaitOnCPU(Core::u64 value) = 0;
    [[nodiscard]] virtual Core::u64 GetCompletedValue() const noexcept = 0;
    [[nodiscard]] virtual void* GetNativeFence() const noexcept = 0;
};

} // namespace Engine::RHI
