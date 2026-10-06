#include "engine/rhi/src/d3d12/D3D12PipelineState.h"

namespace Engine::RHI::D3D12 {

D3D12PipelineState::D3D12PipelineState(ID3D12Device* device, const GraphicsPipelineDesc& desc) {
    ENGINE_ASSERT(device != nullptr, "Device cannot be null");

    // 1. Create a modern bindless Root Signature with push constants
    D3D12_ROOT_PARAMETER rootParameters[1]{};
    
    // Parameter 0: maximum D3D12 root constants, shared by renderer and editor shaders.
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].Constants.ShaderRegister = 0;
    rootParameters[0].Constants.RegisterSpace = 0;
    rootParameters[0].Constants.Num32BitValues = 64;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters = rootParameters;
    rsDesc.NumStaticSamplers = 0;
    rsDesc.pStaticSamplers = nullptr;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                   D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED; // Bindless SM 6.6

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            ENGINE_LOG_ERROR("Root signature serialization failed: {}", static_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        DX_CHECK(hr);
    }

    DX_CHECK(device->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    ));

    // 2. Configure Graphics Pipeline State
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pDesc{};
    pDesc.pRootSignature = m_rootSignature.Get();

    if (desc.vertexShaderBytecode && desc.vertexShaderSize > 0) {
        pDesc.VS.pShaderBytecode = desc.vertexShaderBytecode;
        pDesc.VS.BytecodeLength = desc.vertexShaderSize;
    }

    if (desc.pixelShaderBytecode && desc.pixelShaderSize > 0) {
        pDesc.PS.pShaderBytecode = desc.pixelShaderBytecode;
        pDesc.PS.BytecodeLength = desc.pixelShaderSize;
    }

    // Default Rasterizer
    pDesc.RasterizerState.FillMode = desc.wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    pDesc.RasterizerState.CullMode = desc.cullBackFace ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
    pDesc.RasterizerState.FrontCounterClockwise = FALSE;
    pDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    pDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    pDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    pDesc.RasterizerState.DepthClipEnable = TRUE;

    // Default Blend
    pDesc.BlendState.AlphaToCoverageEnable = FALSE;
    pDesc.BlendState.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRTBlendDesc = {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
        pDesc.BlendState.RenderTarget[i] = defaultRTBlendDesc;
    }

    // Depth Stencil
    pDesc.DepthStencilState.DepthEnable = desc.depthEnable ? TRUE : FALSE;
    pDesc.DepthStencilState.DepthWriteMask = desc.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    pDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    pDesc.DepthStencilState.StencilEnable = FALSE;

    pDesc.SampleMask = UINT_MAX;
    pDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pDesc.NumRenderTargets = desc.renderTargetCount;
    for (Core::u32 i = 0; i < desc.renderTargetCount; ++i) {
        pDesc.RTVFormats[i] = ConvertFormat(desc.renderTargetFormats[i]);
    }
    pDesc.DSVFormat = ConvertFormat(desc.depthStencilFormat);
    pDesc.SampleDesc.Count = 1;
    pDesc.SampleDesc.Quality = 0;

    DX_CHECK(device->CreateGraphicsPipelineState(&pDesc, IID_PPV_ARGS(&m_pso)));
}

} // namespace Engine::RHI::D3D12
