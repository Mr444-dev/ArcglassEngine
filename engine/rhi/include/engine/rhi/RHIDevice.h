#pragma once

#include "engine/rhi/RHITypes.h"
#include "engine/rhi/RHIBuffer.h"
#include "engine/rhi/RHITexture.h"
#include "engine/rhi/RHIPipelineState.h"
#include "engine/rhi/RHICommandList.h"
#include "engine/rhi/RHISwapChain.h"
#include "engine/rhi/RHISync.h"
#include "engine/rhi/RHIDescriptorHeap.h"
#include <memory>

namespace Engine::RHI {

class RHIDevice {
public:
    static RHIDevice& Get();

    virtual ~RHIDevice() = default;

    virtual bool Initialize(void* nativeWindowHandle, Core::u32 width, Core::u32 height) = 0;
    virtual void Shutdown() = 0;

    [[nodiscard]] virtual std::unique_ptr<RHIBuffer> CreateBuffer(const BufferDesc& desc) = 0;
    [[nodiscard]] virtual std::unique_ptr<RHITexture> CreateTexture(const TextureDesc& desc) = 0;
    [[nodiscard]] virtual std::unique_ptr<RHIPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineDesc& desc) = 0;
    [[nodiscard]] virtual std::unique_ptr<RHICommandList> CreateCommandList() = 0;
    [[nodiscard]] virtual std::unique_ptr<RHIFence> CreateFence() = 0;
    [[nodiscard]] virtual std::unique_ptr<RHISwapChain> CreateSwapChain(void* nativeWindowHandle, Core::u32 width, Core::u32 height) = 0;

    virtual void ExecuteCommandList(RHICommandList* commandList) = 0;
    virtual void ExecuteCommandLists(Core::u32 count, RHICommandList* const* commandLists) = 0;

    virtual void WaitForGPU() = 0;

    [[nodiscard]] virtual RHIDescriptorHeap* GetDescriptorHeap() const noexcept = 0;
    [[nodiscard]] virtual void* GetNativeDevice() const noexcept = 0;
};

} // namespace Engine::RHI
