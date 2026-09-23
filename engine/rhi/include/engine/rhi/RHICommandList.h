#pragma once

#include "engine/rhi/RHITypes.h"
#include "engine/rhi/RHIBuffer.h"
#include "engine/rhi/RHITexture.h"
#include "engine/rhi/RHIPipelineState.h"
#include "engine/rhi/RHIDescriptorHeap.h"

namespace Engine::RHI {

class RHICommandList {
public:
    virtual ~RHICommandList() = default;

    virtual void Reset(RHIPipelineState* initialPSO = nullptr) = 0;
    virtual void Close() = 0;

    // Resource Transitions
    virtual void TransitionBarrier(RHITexture* texture, ResourceState targetState) = 0;
    virtual void TransitionBarrier(RHIBuffer* buffer, ResourceState targetState) = 0;

    // Render Target & Viewport Binding
    virtual void SetRenderTargets(Core::u32 count, RHITexture* const* renderTargets, RHITexture* depthStencil) = 0;
    virtual void ClearRenderTarget(RHITexture* renderTarget, float r, float g, float b, float a) = 0;
    virtual void ClearDepthStencil(RHITexture* depthStencil, float depth = 1.0f, Core::u8 stencil = 0) = 0;
    virtual void SetViewport(const Viewport& viewport) = 0;
    virtual void SetScissorRect(const ScissorRect& scissor) = 0;

    // Pipeline & Descriptors
    virtual void SetPipelineState(RHIPipelineState* pso) = 0;
    virtual void SetDescriptorHeap(RHIDescriptorHeap* heap) = 0;
    virtual void SetRoot32BitConstants(Core::u32 rootParameterIndex, Core::u32 numConstants, const void* data, Core::u32 offset) = 0;

    // Draw & Dispatch
    virtual void DrawInstanced(Core::u32 vertexCount, Core::u32 instanceCount, Core::u32 startVertex, Core::u32 startInstance) = 0;
    virtual void DrawIndexedInstanced(Core::u32 indexCount, Core::u32 instanceCount, Core::u32 startIndex, Core::i32 baseVertex, Core::u32 startInstance) = 0;
    virtual void DispatchCompute(Core::u32 threadGroupCountX, Core::u32 threadGroupCountY, Core::u32 threadGroupCountZ) = 0;
    virtual void DispatchMesh(Core::u32 threadGroupCountX, Core::u32 threadGroupCountY, Core::u32 threadGroupCountZ) = 0;
    virtual void ExecuteIndirect(void* commandSignature, Core::u32 maxCommands, RHIBuffer* argumentBuffer, Core::u64 argumentOffset, RHIBuffer* countBuffer = nullptr, Core::u64 countOffset = 0) = 0;

    [[nodiscard]] virtual void* GetNativeCommandList() const noexcept = 0;
};

} // namespace Engine::RHI
