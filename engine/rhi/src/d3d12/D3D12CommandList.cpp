#include "engine/rhi/src/d3d12/D3D12CommandList.h"

namespace Engine::RHI::D3D12 {

D3D12CommandList::D3D12CommandList(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
    : m_type(type)
{
    ENGINE_ASSERT(device != nullptr, "Device cannot be null");
    DX_CHECK(device->CreateCommandAllocator(m_type, IID_PPV_ARGS(&m_allocator)));

    ComPtr<ID3D12GraphicsCommandList> baseCmdList;
    DX_CHECK(device->CreateCommandList(0, m_type, m_allocator.Get(), nullptr, IID_PPV_ARGS(&baseCmdList)));
    DX_CHECK(baseCmdList.As(&m_cmdList));

    // Initially close it so Reset can be called cleanly
    DX_CHECK(m_cmdList->Close());
}

void D3D12CommandList::Reset(RHIPipelineState* initialPSO) {
    DX_CHECK(m_allocator->Reset());
    auto* nativePSO = initialPSO ? static_cast<ID3D12PipelineState*>(initialPSO->GetNativePSO()) : nullptr;
    DX_CHECK(m_cmdList->Reset(m_allocator.Get(), nativePSO));

    if (initialPSO && initialPSO->GetNativeRootSignature()) {
        auto* rootSig = static_cast<ID3D12RootSignature*>(initialPSO->GetNativeRootSignature());
        m_cmdList->SetGraphicsRootSignature(rootSig);
    }
}

void D3D12CommandList::Close() {
    DX_CHECK(m_cmdList->Close());
}

void D3D12CommandList::TransitionBarrier(RHITexture* texture, ResourceState targetState) {
    if (!texture || texture->GetCurrentState() == targetState) return;

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = static_cast<ID3D12Resource*>(texture->GetNativeResource());
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = ConvertResourceState(texture->GetCurrentState());
    barrier.Transition.StateAfter = ConvertResourceState(targetState);

    m_cmdList->ResourceBarrier(1, &barrier);
    texture->SetCurrentState(targetState);
}

void D3D12CommandList::TransitionBarrier(RHIBuffer* buffer, ResourceState targetState) {
    if (!buffer || buffer->GetCurrentState() == targetState) return;

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = static_cast<ID3D12Resource*>(buffer->GetNativeResource());
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = ConvertResourceState(buffer->GetCurrentState());
    barrier.Transition.StateAfter = ConvertResourceState(targetState);

    m_cmdList->ResourceBarrier(1, &barrier);
    buffer->SetCurrentState(targetState);
}

void D3D12CommandList::SetRenderTargets(Core::u32 count, RHITexture* const* renderTargets, RHITexture* depthStencil) {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[8]{};
    Core::u32 validRtvCount = 0;
    for (Core::u32 i = 0; i < count && i < 8; ++i) {
        if (renderTargets && renderTargets[i] && renderTargets[i]->GetRTVDescriptorHandle() != 0) {
            rtvHandles[validRtvCount++].ptr = renderTargets[i]->GetRTVDescriptorHandle();
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{0};
    D3D12_CPU_DESCRIPTOR_HANDLE* pDSV = nullptr;
    if (depthStencil && depthStencil->GetDSVDescriptorHandle() != 0) {
        dsvHandle.ptr = depthStencil->GetDSVDescriptorHandle();
        pDSV = &dsvHandle;
    }

    if (validRtvCount > 0 || pDSV != nullptr) {
        m_cmdList->OMSetRenderTargets(validRtvCount, rtvHandles, FALSE, pDSV);
    }
}

void D3D12CommandList::ClearRenderTarget(RHITexture* renderTarget, float r, float g, float b, float a) {
    if (!renderTarget || renderTarget->GetRTVDescriptorHandle() == 0) return;
    const float clearColor[4] = { r, g, b, a };
    D3D12_CPU_DESCRIPTOR_HANDLE handle{ renderTarget->GetRTVDescriptorHandle() };
    m_cmdList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
}

void D3D12CommandList::ClearDepthStencil(RHITexture* depthStencil, float depth, Core::u8 stencil) {
    if (!depthStencil || depthStencil->GetDSVDescriptorHandle() == 0) return;
    D3D12_CPU_DESCRIPTOR_HANDLE handle{ depthStencil->GetDSVDescriptorHandle() };
    m_cmdList->ClearDepthStencilView(handle, D3D12_CLEAR_FLAG_DEPTH, depth, stencil, 0, nullptr);
}

void D3D12CommandList::SetViewport(const Viewport& viewport) {
    D3D12_VIEWPORT vp{};
    vp.TopLeftX = viewport.x;
    vp.TopLeftY = viewport.y;
    vp.Width = viewport.width;
    vp.Height = viewport.height;
    vp.MinDepth = viewport.minDepth;
    vp.MaxDepth = viewport.maxDepth;
    m_cmdList->RSSetViewports(1, &vp);
}

void D3D12CommandList::SetScissorRect(const ScissorRect& scissor) {
    D3D12_RECT rect{};
    rect.left = scissor.left;
    rect.top = scissor.top;
    rect.right = scissor.right;
    rect.bottom = scissor.bottom;
    m_cmdList->RSSetScissorRects(1, &rect);
}

void D3D12CommandList::SetPipelineState(RHIPipelineState* pso) {
    if (!pso) return;
    m_cmdList->SetPipelineState(static_cast<ID3D12PipelineState*>(pso->GetNativePSO()));
    if (pso->GetNativeRootSignature()) {
        m_cmdList->SetGraphicsRootSignature(static_cast<ID3D12RootSignature*>(pso->GetNativeRootSignature()));
    }
}

void D3D12CommandList::SetDescriptorHeap(RHIDescriptorHeap* heap) {
    if (!heap) return;
    ID3D12DescriptorHeap* heaps[1] = { static_cast<ID3D12DescriptorHeap*>(heap->GetNativeHeap()) };
    m_cmdList->SetDescriptorHeaps(1, heaps);
}

void D3D12CommandList::SetRoot32BitConstants(Core::u32 rootParameterIndex, Core::u32 numConstants, const void* data, Core::u32 offset) {
    m_cmdList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, data, offset);
}

void D3D12CommandList::DrawInstanced(Core::u32 vertexCount, Core::u32 instanceCount, Core::u32 startVertex, Core::u32 startInstance) {
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmdList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void D3D12CommandList::DrawIndexedInstanced(Core::u32 indexCount, Core::u32 instanceCount, Core::u32 startIndex, Core::i32 baseVertex, Core::u32 startInstance) {
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmdList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void D3D12CommandList::DispatchCompute(Core::u32 threadGroupCountX, Core::u32 threadGroupCountY, Core::u32 threadGroupCountZ) {
    m_cmdList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void D3D12CommandList::DispatchMesh(Core::u32 threadGroupCountX, Core::u32 threadGroupCountY, Core::u32 threadGroupCountZ) {
    m_cmdList->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void D3D12CommandList::ExecuteIndirect(void* commandSignature, Core::u32 maxCommands, RHIBuffer* argumentBuffer, Core::u64 argumentOffset, RHIBuffer* countBuffer, Core::u64 countOffset) {
    auto* sig = static_cast<ID3D12CommandSignature*>(commandSignature);
    auto* argRes = static_cast<ID3D12Resource*>(argumentBuffer->GetNativeResource());
    auto* cntRes = countBuffer ? static_cast<ID3D12Resource*>(countBuffer->GetNativeResource()) : nullptr;
    m_cmdList->ExecuteIndirect(sig, maxCommands, argRes, argumentOffset, cntRes, countOffset);
}

} // namespace Engine::RHI::D3D12
