#pragma once

#include "engine/rhi/RHICommandList.h"
#include "engine/rhi/src/d3d12/D3D12Common.h"

namespace Engine::RHI::D3D12 {

class D3D12CommandList final : public RHICommandList {
public:
    explicit D3D12CommandList(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
    ~D3D12CommandList() override = default;

    void Reset(RHIPipelineState* initialPSO = nullptr) override;
    void Close() override;

    void TransitionBarrier(RHITexture* texture, ResourceState targetState) override;
    void TransitionBarrier(RHIBuffer* buffer, ResourceState targetState) override;

    void SetRenderTargets(Core::u32 count, RHITexture* const* renderTargets, RHITexture* depthStencil) override;
    void ClearRenderTarget(RHITexture* renderTarget, float r, float g, float b, float a) override;
    void ClearDepthStencil(RHITexture* depthStencil, float depth = 1.0f, Core::u8 stencil = 0) override;
    void SetViewport(const Viewport& viewport) override;
    void SetScissorRect(const ScissorRect& scissor) override;

    void SetPipelineState(RHIPipelineState* pso) override;
    void SetDescriptorHeap(RHIDescriptorHeap* heap) override;
    void SetRoot32BitConstants(Core::u32 rootParameterIndex, Core::u32 numConstants, const void* data, Core::u32 offset) override;

    void DrawInstanced(Core::u32 vertexCount, Core::u32 instanceCount, Core::u32 startVertex, Core::u32 startInstance) override;
    void DrawIndexedInstanced(Core::u32 indexCount, Core::u32 instanceCount, Core::u32 startIndex, Core::i32 baseVertex, Core::u32 startInstance) override;
    void DispatchCompute(Core::u32 threadGroupCountX, Core::u32 threadGroupCountY, Core::u32 threadGroupCountZ) override;
    void DispatchMesh(Core::u32 threadGroupCountX, Core::u32 threadGroupCountY, Core::u32 threadGroupCountZ) override;
    void ExecuteIndirect(void* commandSignature, Core::u32 maxCommands, RHIBuffer* argumentBuffer, Core::u64 argumentOffset, RHIBuffer* countBuffer = nullptr, Core::u64 countOffset = 0) override;

    [[nodiscard]] void* GetNativeCommandList() const noexcept override { return m_cmdList.Get(); }
    [[nodiscard]] ID3D12CommandAllocator* GetNativeAllocator() const noexcept { return m_allocator.Get(); }

private:
    ComPtr<ID3D12CommandAllocator> m_allocator;
    ComPtr<ID3D12GraphicsCommandList6> m_cmdList;
    D3D12_COMMAND_LIST_TYPE m_type{D3D12_COMMAND_LIST_TYPE_DIRECT};
};

} // namespace Engine::RHI::D3D12
