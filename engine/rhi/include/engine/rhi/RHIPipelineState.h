#pragma once

#include "engine/rhi/RHITypes.h"
#include <vector>

namespace Engine::RHI {

enum class PipelineType : Core::u8 {
    Graphics,
    Compute,
    Mesh
};

struct GraphicsPipelineDesc {
    const void* vertexShaderBytecode{nullptr};
    size_t vertexShaderSize{0};

    const void* pixelShaderBytecode{nullptr};
    size_t pixelShaderSize{0};

    const void* amplificationShaderBytecode{nullptr};
    size_t amplificationShaderSize{0};

    const void* meshShaderBytecode{nullptr};
    size_t meshShaderSize{0};

    Core::u32 renderTargetCount{1};
    Format renderTargetFormats[8]{Format::RGBA8_UNORM};
    Format depthStencilFormat{Format::D32_FLOAT};

    bool depthEnable{true};
    bool depthWrite{true};
    bool cullBackFace{true};
    bool wireframe{false};
};

class RHIPipelineState {
public:
    virtual ~RHIPipelineState() = default;

    [[nodiscard]] virtual PipelineType GetType() const noexcept = 0;
    [[nodiscard]] virtual void* GetNativePSO() const noexcept = 0;
    [[nodiscard]] virtual void* GetNativeRootSignature() const noexcept = 0;
};

} // namespace Engine::RHI
