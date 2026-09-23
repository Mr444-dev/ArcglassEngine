#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/rhi/RHITypes.h"
#include "engine/rhi/RHITexture.h"
#include "engine/rhi/RHIBuffer.h"
#include <string>

namespace Engine::Renderer {

struct RGTextureHandle {
    Core::u32 id{0xFFFFFFFF};
    Core::u32 generation{0};

    [[nodiscard]] bool IsValid() const noexcept { return id != 0xFFFFFFFF; }
    [[nodiscard]] bool operator==(const RGTextureHandle& o) const noexcept { return id == o.id && generation == o.generation; }
    [[nodiscard]] bool operator!=(const RGTextureHandle& o) const noexcept { return !(*this == o); }
};

struct RGBufferHandle {
    Core::u32 id{0xFFFFFFFF};
    Core::u32 generation{0};

    [[nodiscard]] bool IsValid() const noexcept { return id != 0xFFFFFFFF; }
    [[nodiscard]] bool operator==(const RGBufferHandle& o) const noexcept { return id == o.id && generation == o.generation; }
    [[nodiscard]] bool operator!=(const RGBufferHandle& o) const noexcept { return !(*this == o); }
};

struct RGTextureDesc {
    Core::u32 width{1280};
    Core::u32 height{720};
    RHI::Format format{RHI::Format::RGBA8_UNORM};
    RHI::TextureUsage usage{RHI::TextureUsage::RenderTarget};
    std::string name;
};

struct RGBufferDesc {
    Core::u64 sizeInBytes{0};
    RHI::BufferUsage usage{RHI::BufferUsage::None};
    std::string name;
};

struct RGTextureResource {
    RGTextureDesc desc;
    RHI::RHITexture* physicalTexture{nullptr};
    RHI::ResourceState currentState{RHI::ResourceState::Common};
    bool isImported{false};
    bool isOutput{false};
    Core::u32 refCount{0};
};

struct RGBufferResource {
    RGBufferDesc desc;
    RHI::RHIBuffer* physicalBuffer{nullptr};
    RHI::ResourceState currentState{RHI::ResourceState::Common};
    bool isImported{false};
    bool isOutput{false};
    Core::u32 refCount{0};
};

} // namespace Engine::Renderer
