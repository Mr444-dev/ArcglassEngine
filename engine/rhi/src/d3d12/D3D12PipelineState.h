#pragma once

#include "engine/rhi/RHIPipelineState.h"
#include "engine/rhi/src/d3d12/D3D12Common.h"

namespace Engine::RHI::D3D12 {

class D3D12PipelineState final : public RHIPipelineState {
public:
    D3D12PipelineState(ID3D12Device* device, const GraphicsPipelineDesc& desc);
    ~D3D12PipelineState() override = default;

    [[nodiscard]] PipelineType GetType() const noexcept override { return m_type; }
    [[nodiscard]] void* GetNativePSO() const noexcept override { return m_pso.Get(); }
    [[nodiscard]] void* GetNativeRootSignature() const noexcept override { return m_rootSignature.Get(); }

private:
    PipelineType m_type{PipelineType::Graphics};
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
};

} // namespace Engine::RHI::D3D12
